// SPDX-License-Identifier: GPL-2.0-only
/*
 * Gunyah message queue transport for MiTEE on Qualcomm platforms.
 *
 * The TEE runs in the Gunyah OEM VM. Calls are exchanged with it over a
 * Gunyah message queue, and memory is shared or lent to it through the
 * Gunyah resource manager.
 *
 * This file is divided into the following sections:
 * 1. Wire format and driver state
 * 2. Maintain a hash table for lookup of a Gunyah memparcel handle
 * 3. Convert between struct tee_param and struct optee_msg_param
 * 4. Share and lend memory with the TEE VM
 * 5. Receive thread and calls into secure world
 * 6. Driver initialization
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": [%s %d] " fmt, __func__, __LINE__

#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/firmware/qcom/qcom_scm.h>
#include <linux/gunyah/gh_common.h>
#include <linux/gunyah/gh_msgq.h>
#include <linux/gunyah/gh_rm_drv.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/rhashtable.h>
#include <linux/scatterlist.h>
#include <linux/sched/signal.h>
#include <linux/sizes.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <soc/qcom/secure_buffer.h>

#include <tee_drv.h>
#include "optee_ffa.h"
#include "optee_private.h"
#include "rpc_callback.h"
#include "dynamic_mem.h"

/*
 * 1. Wire format and driver state
 */
#define MSGQ_MAGIC		0x5147534d4f414958ULL /* "XIAOMSGQ" */
#define MSGQ_PEER_REE		0x8000

enum msgq_msg_type {
	MSGQ_MSG_CTRL = 0,
	MSGQ_MSG_DATA = 1,
	MSGQ_MSG_FAST_REQ = 2,
	MSGQ_MSG_YIELD_REQ = 3,
	MSGQ_MSG_YIELD_RESP = 4,
	MSGQ_MSG_FAST_RESP = 5,
};

/* control messages sent by the TEE during bring-up */
enum msgq_ctrl_id {
	MSGQ_CTRL_HELLO = 0,
	MSGQ_CTRL_GET_BUFFERS = 1,
	MSGQ_CTRL_READY = 2,
};

/* buffer setup commands the REE sends back for MSGQ_CTRL_GET_BUFFERS */
#define MSGQ_SET_STATIC_SHM	5
#define MSGQ_SET_MSG_BUFFER	OPTEE_FFA_SET_MITEE_MSG_BUFFER

/* first part of the static shared buffer, the rest is the TEE log */
#define MSGQ_STATIC_SHM_SIZE	SZ_2M

#define MSGQ_REPLY_SLOTS	32

struct msgq_msg_hdr {
	u64 magic;
	u32 type;
	u32 len;	/* payload length */
	u16 src;
	u16 dst;
	u32 msg_id;	/* enum msgq_ctrl_id for MSGQ_MSG_CTRL */
	u64 reserved[2];
} __packed;

struct msgq_msg {
	struct msgq_msg_hdr hdr;
	u64 data[5];	/* cmd/resp + 4 params */
} __packed;

#define MSGQ_PAYLOAD_SIZE	sizeof_field(struct msgq_msg, data)

enum msgq_state {
	MSGQ_DOWN,	/* TEE VM not running */
	MSGQ_VM_UP,	/* TEE VM running, queue not connected */
	MSGQ_CONNECTED,	/* hello exchanged */
	MSGQ_READY,	/* TEE finished bring-up */
};

struct msgq_reply {
	u64 data[5];
};

static struct {
	struct device *dev;
	void *msgq;
	struct task_struct *recv_task;
	struct notifier_block rm_nb;
	enum msgq_state state;

	gh_vmid_t self_vmid;
	gh_vmid_t peer_vmid;
	u32 peer_name;

	/* static shared buffer from DT, shared once the TEE VM is up */
	struct resource shm;
	u32 shm_label;
	gh_memparcel_handle_t shm_handle;
	bool shm_shared;

	/* message buffer provided by the core with SET_MITEE_MSG_BUFFER */
	phys_addr_t msg_buf_pa;
	size_t msg_buf_size;
	gh_memparcel_handle_t msg_buf_handle;
	bool msg_buf_shared;

	/* replies to yielding calls, FIFO */
	spinlock_t reply_lock;
	struct completion reply_done;
	struct msgq_reply replies[MSGQ_REPLY_SLOTS];
	unsigned int reply_head;
	unsigned int reply_tail;

	/* reply to the single outstanding fast call */
	struct mutex fast_mutex;
	spinlock_t fast_lock;
	struct completion fast_done;
	struct msgq_reply fast_reply;
	bool fast_valid;

	struct mutex shm_mutex;
	struct rhashtable shm_handles;

	struct mutex secmem_mutex;
	struct list_head secmem_list;
} msgq_ctx;

/*
 * 2. Maintain a hash table for lookup of a Gunyah memparcel handle
 *
 * Registered shared memory is referred to by its memparcel handle when
 * communicating with secure world.
 */
struct shm_rhash {
	struct tee_shm *shm;
	struct sg_table *sgt;
	u64 handle;
	struct rhash_head linkage;
};

static void rh_free_fn(void *ptr, void *arg)
{
	struct shm_rhash *r = ptr;

	sg_free_table(r->sgt);
	kfree(r->sgt);
	kfree(r);
}

static const struct rhashtable_params shm_rhash_params = {
	.head_offset = offsetof(struct shm_rhash, linkage),
	.key_len = sizeof(u64),
	.key_offset = offsetof(struct shm_rhash, handle),
	.automatic_shrinking = true,
};

static struct tee_shm *msgq_shm_from_handle(u64 handle)
{
	struct tee_shm *shm = NULL;
	struct shm_rhash *r;

	mutex_lock(&msgq_ctx.shm_mutex);
	r = rhashtable_lookup_fast(&msgq_ctx.shm_handles, &handle,
				   shm_rhash_params);
	if (r)
		shm = r->shm;
	mutex_unlock(&msgq_ctx.shm_mutex);

	return shm;
}

static int msgq_shm_add_handle(struct tee_shm *shm, struct sg_table *sgt,
			       u64 handle)
{
	struct shm_rhash *r;
	int rc;

	r = kmalloc(sizeof(*r), GFP_KERNEL);
	if (!r)
		return -ENOMEM;
	r->shm = shm;
	r->sgt = sgt;
	r->handle = handle;

	mutex_lock(&msgq_ctx.shm_mutex);
	rc = rhashtable_lookup_insert_fast(&msgq_ctx.shm_handles, &r->linkage,
					   shm_rhash_params);
	mutex_unlock(&msgq_ctx.shm_mutex);

	if (rc)
		kfree(r);

	return rc;
}

/* returns the sg_table that was registered with @handle, or NULL */
static struct sg_table *msgq_shm_rem_handle(u64 handle)
{
	struct sg_table *sgt = NULL;
	struct shm_rhash *r;

	mutex_lock(&msgq_ctx.shm_mutex);
	r = rhashtable_lookup_fast(&msgq_ctx.shm_handles, &handle,
				   shm_rhash_params);
	if (r && !rhashtable_remove_fast(&msgq_ctx.shm_handles, &r->linkage,
					 shm_rhash_params)) {
		sgt = r->sgt;
		kfree(r);
	}
	mutex_unlock(&msgq_ctx.shm_mutex);

	return sgt;
}

/*
 * 3. Convert between struct tee_param and struct optee_msg_param
 *
 * Registered memory uses the same fmem layout as the FF-A ABI, with the
 * Gunyah memparcel handle as global id.
 */
static void from_msg_param_msgq_mem(struct optee *optee, struct tee_param *p,
				    u32 attr, const struct optee_msg_param *mp)
{
	struct tee_shm *shm = NULL;
	u64 offs_high = 0;
	u64 offs_low = 0;

	p->attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT + attr -
		  OPTEE_MSG_ATTR_TYPE_FMEM_INPUT;
	p->u.memref.size = mp->u.fmem.size;

	if (mp->u.fmem.global_id != OPTEE_MSG_FMEM_INVALID_GLOBAL_ID)
		shm = msgq_shm_from_handle(mp->u.fmem.global_id);
	p->u.memref.shm = shm;

	if (shm) {
		offs_low = mp->u.fmem.offs_low;
		offs_high = mp->u.fmem.offs_high;
	}
	p->u.memref.shm_offs = offs_low | offs_high << 32;
}

static int to_msg_param_msgq_mem(struct optee_msg_param *mp,
				 const struct tee_param *p)
{
	struct tee_shm *shm = p->u.memref.shm;

	mp->attr = OPTEE_MSG_ATTR_TYPE_FMEM_INPUT + p->attr -
		   TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT;

	if (shm) {
		u64 shm_offs = p->u.memref.shm_offs;

		mp->u.fmem.internal_offs = shm->offset;
		mp->u.fmem.offs_low = shm_offs;
		mp->u.fmem.offs_high = shm_offs >> 32;
		/* Check that the entire offset could be stored. */
		if (mp->u.fmem.offs_high != shm_offs >> 32)
			return -EINVAL;

		mp->u.fmem.global_id = shm->sec_world_id;
	} else {
		memset(&mp->u, 0, sizeof(mp->u));
		mp->u.fmem.global_id = OPTEE_MSG_FMEM_INVALID_GLOBAL_ID;
	}
	mp->u.fmem.size = p->u.memref.size;

	return 0;
}

/*
 * 4. Share and lend memory with the TEE VM
 */

/* stage 2 permissions of @sgt: self only, or self and the TEE VM */
static int msgq_assign_shared(struct sg_table *sgt, bool share)
{
	u32 self = msgq_ctx.self_vmid;
	u32 both[] = { msgq_ctx.self_vmid, msgq_ctx.peer_vmid };
	int dst_both[] = { msgq_ctx.self_vmid, msgq_ctx.peer_vmid };
	int perm_both[] = { PERM_READ | PERM_WRITE, PERM_READ | PERM_WRITE };
	int dst_self = msgq_ctx.self_vmid;
	int perm_self = PERM_READ | PERM_WRITE | PERM_EXEC;
	int rc;

	if (share)
		rc = hyp_assign_table(sgt, &self, 1, dst_both, perm_both, 2);
	else
		rc = hyp_assign_table(sgt, both, 2, &dst_self, &perm_self, 1);
	if (rc)
		pr_err("hyp_assign_table failed: %d\n", rc);

	return rc;
}

/* move the stage 2 ownership of @sgt from @src to @dst */
static int msgq_assign_lent(struct sg_table *sgt, u32 src, u32 dst)
{
	int dst_vm = dst;
	int perm = PERM_READ | PERM_WRITE | PERM_EXEC;
	int rc;

	rc = hyp_assign_table(sgt, &src, 1, &dst_vm, &perm, 1);
	if (rc)
		pr_err("hyp_assign_table failed: %d\n", rc);

	return rc;
}

static struct gh_acl_desc *msgq_acl_alloc(bool lend)
{
	struct gh_acl_desc *acl;
	u32 n = lend ? 1 : 2;

	acl = kzalloc(struct_size(acl, acl_entries, n), GFP_KERNEL);
	if (!acl)
		return NULL;

	acl->n_acl_entries = n;
	if (lend) {
		acl->acl_entries[0].vmid = msgq_ctx.peer_vmid;
		acl->acl_entries[0].perms = GH_RM_ACL_R | GH_RM_ACL_W |
					    GH_RM_ACL_X;
	} else {
		acl->acl_entries[0].vmid = msgq_ctx.self_vmid;
		acl->acl_entries[0].perms = GH_RM_ACL_R | GH_RM_ACL_W;
		acl->acl_entries[1].vmid = msgq_ctx.peer_vmid;
		acl->acl_entries[1].perms = GH_RM_ACL_R | GH_RM_ACL_W;
	}

	return acl;
}

static struct gh_sgl_desc *msgq_sgl_from_sgt(struct sg_table *sgt)
{
	struct gh_sgl_desc *sgl;
	struct scatterlist *sg;
	int i;

	sgl = kzalloc(struct_size(sgl, sgl_entries, sgt->orig_nents),
		      GFP_KERNEL);
	if (!sgl)
		return NULL;

	sgl->n_sgl_entries = sgt->orig_nents;
	for_each_sgtable_sg(sgt, sg, i) {
		sgl->sgl_entries[i].ipa_base = sg_phys(sg);
		sgl->sgl_entries[i].size = sg->length;
	}

	return sgl;
}

/* share a physically contiguous region (static buffers) with the TEE VM */
static int msgq_share_contiguous(phys_addr_t pa, size_t size, u32 label,
				 gh_memparcel_handle_t *handle)
{
	u64 src_self = BIT(msgq_ctx.self_vmid);
	u64 src_both = BIT(msgq_ctx.self_vmid) | BIT(msgq_ctx.peer_vmid);
	struct qcom_scm_vmperm both[] = {
		{ msgq_ctx.self_vmid, QCOM_SCM_PERM_RW },
		{ msgq_ctx.peer_vmid, QCOM_SCM_PERM_RW },
	};
	struct qcom_scm_vmperm self = {
		msgq_ctx.self_vmid, QCOM_SCM_PERM_RWX
	};
	struct gh_acl_desc *acl;
	struct gh_sgl_desc *sgl;
	int rc;

	rc = qcom_scm_assign_mem(pa, size, &src_self, both, ARRAY_SIZE(both));
	if (rc) {
		pr_err("assign 0x%llx+0x%zx failed: %d\n", (u64)pa, size, rc);
		return rc;
	}

	acl = msgq_acl_alloc(false);
	sgl = kzalloc(struct_size(sgl, sgl_entries, 1), GFP_KERNEL);
	if (!acl || !sgl) {
		rc = -ENOMEM;
		goto err_assign_back;
	}
	sgl->n_sgl_entries = 1;
	sgl->sgl_entries[0].ipa_base = pa;
	sgl->sgl_entries[0].size = size;

	rc = ghd_rm_mem_share(GH_RM_MEM_TYPE_NORMAL, 0, label, acl, sgl, NULL,
			      handle);
	if (rc) {
		pr_err("share 0x%llx+0x%zx failed: %d\n", (u64)pa, size, rc);
		goto err_assign_back;
	}

	kfree(sgl);
	kfree(acl);
	return 0;

err_assign_back:
	kfree(sgl);
	kfree(acl);
	qcom_scm_assign_mem(pa, size, &src_both, &self, 1);
	return rc;
}

static void msgq_unshare_contiguous(phys_addr_t pa, size_t size,
				    gh_memparcel_handle_t handle)
{
	u64 src_both = BIT(msgq_ctx.self_vmid) | BIT(msgq_ctx.peer_vmid);
	struct qcom_scm_vmperm self = {
		msgq_ctx.self_vmid, QCOM_SCM_PERM_RWX
	};
	int rc;

	rc = ghd_rm_mem_reclaim(handle, 0);
	if (rc)
		pr_err("reclaim 0x%x failed: %d\n", handle, rc);

	qcom_scm_assign_mem(pa, size, &src_both, &self, 1);
}

/* take the static buffers back from a TEE VM that went away */
static void msgq_unshare_static(void)
{
	if (msgq_ctx.msg_buf_shared) {
		msgq_unshare_contiguous(msgq_ctx.msg_buf_pa,
					msgq_ctx.msg_buf_size,
					msgq_ctx.msg_buf_handle);
		msgq_ctx.msg_buf_shared = false;
	}
	if (msgq_ctx.shm_shared) {
		msgq_unshare_contiguous(msgq_ctx.shm.start,
					resource_size(&msgq_ctx.shm),
					msgq_ctx.shm_handle);
		msgq_ctx.shm_shared = false;
	}
}

static int optee_msgq_shm_register(struct tee_context *ctx,
				   struct tee_shm *shm, struct page **pages,
				   size_t num_pages, unsigned long start)
{
	struct optee *optee = tee_get_drvdata(ctx->teedev);
	gh_memparcel_handle_t handle;
	struct gh_acl_desc *acl;
	struct gh_sgl_desc *sgl;
	struct sg_table *sgt;
	int rc;

	if (optee->supp_teedev == ctx->teedev) {
		pr_err("do not support supp!\n");
		return -EACCES;
	}

	rc = optee_check_mem_type(start, num_pages);
	if (rc) {
		pr_err("optee_check_mem_type is failed\n");
		return rc;
	}

	sgt = kzalloc(sizeof(*sgt), GFP_KERNEL);
	if (!sgt)
		return -ENOMEM;

	rc = sg_alloc_table_from_pages(sgt, pages, num_pages, 0,
				       num_pages * PAGE_SIZE, GFP_KERNEL);
	if (rc)
		goto err_free_sgt;

	rc = msgq_assign_shared(sgt, true);
	if (rc)
		goto err_free_table;

	acl = msgq_acl_alloc(false);
	sgl = msgq_sgl_from_sgt(sgt);
	if (acl && sgl)
		rc = ghd_rm_mem_share(GH_RM_MEM_TYPE_NORMAL, 0, 0, acl, sgl,
				      NULL, &handle);
	else
		rc = -ENOMEM;
	kfree(sgl);
	kfree(acl);
	if (rc) {
		pr_err("memory share failed: %d\n", rc);
		goto err_assign_back;
	}

	rc = msgq_shm_add_handle(shm, sgt, handle);
	if (rc) {
		pr_err("add handle failed: %d\n", rc);
		ghd_rm_mem_reclaim(handle, 0);
		goto err_assign_back;
	}

	shm->sec_world_id = handle;
	return 0;

err_assign_back:
	msgq_assign_shared(sgt, false);
err_free_table:
	sg_free_table(sgt);
err_free_sgt:
	kfree(sgt);
	return rc;
}

static int msgq_shm_release(u64 handle)
{
	struct sg_table *sgt;
	int rc;

	sgt = msgq_shm_rem_handle(handle);

	rc = ghd_rm_mem_reclaim(handle, 0);
	if (rc)
		pr_err("reclaim 0x%llx failed: %d\n", handle, rc);

	if (sgt) {
		msgq_assign_shared(sgt, false);
		sg_free_table(sgt);
		kfree(sgt);
	}

	return rc;
}

static int optee_msgq_shm_unregister(struct tee_context *ctx,
				     struct tee_shm *shm)
{
	struct optee *optee = tee_get_drvdata(ctx->teedev);
	u64 handle = shm->sec_world_id;
	struct optee_msg_arg *msg_arg;
	int rc;

	msg_arg = kzalloc(OPTEE_MSG_GET_ARG_SIZE(1), GFP_KERNEL);
	if (!msg_arg)
		return -ENOMEM;
	msg_arg->num_params = 1;
	msg_arg->cmd = OPTEE_MSG_CMD_UNREGISTER_SHM;
	msg_arg->params[0].attr = OPTEE_MSG_ATTR_TYPE_FMEM_INPUT;
	msg_arg->params[0].u.fmem.global_id = handle;

	rc = optee->ops->do_call_with_arg(ctx, msg_arg);
	if (rc)
		pr_err("Unregister SHM id 0x%llx rc %d\n", handle, rc);
	kfree(msg_arg);

	shm->sec_world_id = 0;
	return msgq_shm_release(handle);
}

static int optee_msgq_shm_unregister_supp(struct tee_context *ctx,
					  struct tee_shm *shm)
{
	u64 handle = shm->sec_world_id;

	/*
	 * OP-TEE is freeing this memory via RPC, it has already retired the
	 * handle, so there is no unregister call to make.
	 */
	shm->sec_world_id = 0;
	return msgq_shm_release(handle);
}

/* memory lent to the TEE on its request, see mitee_msgq_rpc_callback() */
struct msgq_secmem {
	struct list_head node;
	struct sg_table *sgt;
	u64 handle;
	u32 size;
};

static int msgq_secmem_lend(u32 size, u64 *handle_out)
{
	gh_memparcel_handle_t handle;
	struct msgq_secmem *mem;
	struct gh_acl_desc *acl;
	struct gh_sgl_desc *sgl;
	struct sg_table *sgt;
	int rc;

	mem = kzalloc(sizeof(*mem), GFP_KERNEL);
	if (!mem)
		return -ENOMEM;

	rc = mitee_alloc_memory_sgt(size, &sgt);
	if (rc)
		goto err_free_mem;

	rc = msgq_assign_lent(sgt, msgq_ctx.self_vmid, msgq_ctx.peer_vmid);
	if (rc)
		goto err_free_sgt;

	acl = msgq_acl_alloc(true);
	sgl = msgq_sgl_from_sgt(sgt);
	if (acl && sgl)
		rc = ghd_rm_mem_lend(GH_RM_MEM_TYPE_NORMAL, 0, 0, acl, sgl,
				     NULL, &handle);
	else
		rc = -ENOMEM;
	kfree(sgl);
	kfree(acl);
	if (rc) {
		pr_err("memory lend failed: %d\n", rc);
		goto err_assign_back;
	}

	mem->sgt = sgt;
	mem->handle = handle;
	mem->size = size;
	mutex_lock(&msgq_ctx.secmem_mutex);
	list_add_tail(&mem->node, &msgq_ctx.secmem_list);
	mutex_unlock(&msgq_ctx.secmem_mutex);

	*handle_out = handle;
	return 0;

err_assign_back:
	msgq_assign_lent(sgt, msgq_ctx.peer_vmid, msgq_ctx.self_vmid);
err_free_sgt:
	mitee_free_memory_sgt(size, sgt);
err_free_mem:
	kfree(mem);
	return rc;
}

static int msgq_secmem_reclaim(u64 handle)
{
	struct msgq_secmem *mem = NULL, *iter;
	int rc;

	mutex_lock(&msgq_ctx.secmem_mutex);
	list_for_each_entry(iter, &msgq_ctx.secmem_list, node) {
		if (iter->handle == handle) {
			list_del(&iter->node);
			mem = iter;
			break;
		}
	}
	mutex_unlock(&msgq_ctx.secmem_mutex);

	if (!mem) {
		pr_err("unknown handle 0x%llx\n", handle);
		return -EINVAL;
	}

	rc = ghd_rm_mem_reclaim(handle, 0);
	if (rc) {
		/* still owned by the TEE VM, don't hand it back to the kernel */
		pr_err("reclaim 0x%llx failed: %d\n", handle, rc);
		return rc;
	}

	msgq_assign_lent(mem->sgt, msgq_ctx.peer_vmid, msgq_ctx.self_vmid);
	mitee_free_memory_sgt(mem->size, mem->sgt);
	kfree(mem);
	return 0;
}

static uint32_t mitee_msgq_rpc_callback(struct optee_msg_param_value *value,
					void *buf, uint32_t size_in,
					uint32_t *size_out)
{
	u32 module_id = value->b;
	u32 sub_cmd = value->a;
	u64 handle;
	u32 size;

	if (module_id != REE_CALLBACK_MODULE_TEE_FRAMEWORK)
		return TEEC_ERROR_BAD_PARAMETERS;

	switch (sub_cmd) {
	case OPTEE_REE_CALLBACK_ALLOCATE_SECMEM:
		size = PAGE_ALIGN(value->c);
		if (msgq_secmem_lend(size, &handle))
			return TEEC_ERROR_BAD_PARAMETERS;
		value->a = handle;
		value->b = value->c;
		return 0;
	case OPTEE_REE_CALLBACK_FREE_SECMEM:
		msgq_secmem_reclaim(value->c);
		return 0;
	default:
		pr_err("unknown cmd 0x%x\n", sub_cmd);
		return TEEC_ERROR_BAD_PARAMETERS;
	}
}

/*
 * 5. Receive thread and calls into secure world
 */
static int msgq_send(struct msgq_msg *msg, u32 type, u32 len)
{
	msg->hdr.magic = MSGQ_MAGIC;
	msg->hdr.type = type;
	msg->hdr.len = len;

	return gh_msgq_send(msgq_ctx.msgq, msg, sizeof(msg->hdr) + len, 0);
}

/* a new request from the REE, as opposed to a reply to a control message */
static int msgq_send_request(u32 type, const u64 *data)
{
	struct msgq_msg msg = {
		.hdr.src = MSGQ_PEER_REE,
	};

	memcpy(msg.data, data, MSGQ_PAYLOAD_SIZE);

	return msgq_send(&msg, type, MSGQ_PAYLOAD_SIZE);
}

/* control messages are answered in place, keeping the TEE's header ids */
static void msgq_handle_ctrl(struct msgq_msg *msg)
{
	int rc;

	if (msg->hdr.dst != MSGQ_PEER_REE) {
		pr_err("unknown server id 0x%x\n", msg->hdr.dst);
		return;
	}
	if (msg->hdr.len) {
		pr_err("unexpected ctrl payload %u\n", msg->hdr.len);
		return;
	}
	msg->hdr.src |= MSGQ_PEER_REE;

	switch (msg->hdr.msg_id) {
	case MSGQ_CTRL_HELLO:
		if (msgq_ctx.state >= MSGQ_CONNECTED)
			pr_err("hello in state %d\n", msgq_ctx.state);
		msgq_send(msg, MSGQ_MSG_CTRL, 0);
		msgq_ctx.state = MSGQ_CONNECTED;
		break;
	case MSGQ_CTRL_GET_BUFFERS:
		msg->data[0] = MSGQ_SET_STATIC_SHM;
		msg->data[1] = msgq_ctx.shm.start;
		msg->data[2] = MSGQ_STATIC_SHM_SIZE;
		msg->data[3] = msgq_ctx.shm_handle;
		msg->data[4] = msgq_ctx.shm_label;
		msgq_send(msg, MSGQ_MSG_FAST_REQ, MSGQ_PAYLOAD_SIZE);

		/* give the TEE time to map the static buffer first */
		usleep_range(5000, 6000);

		if (!msgq_ctx.msg_buf_shared) {
			rc = msgq_share_contiguous(msgq_ctx.msg_buf_pa,
						   msgq_ctx.msg_buf_size, 0,
						   &msgq_ctx.msg_buf_handle);
			if (rc)
				break;
			msgq_ctx.msg_buf_shared = true;
		}
		msg->data[0] = MSGQ_SET_MSG_BUFFER;
		msg->data[1] = msgq_ctx.msg_buf_pa;
		msg->data[2] = msgq_ctx.msg_buf_size;
		msg->data[3] = msgq_ctx.msg_buf_handle;
		msg->data[4] = 0;
		msgq_send(msg, MSGQ_MSG_FAST_REQ, MSGQ_PAYLOAD_SIZE);
		break;
	case MSGQ_CTRL_READY:
		msgq_ctx.state = MSGQ_READY;
		break;
	default:
		pr_info("unknown ctrl id 0x%x\n", msg->hdr.msg_id);
		break;
	}
}

static void msgq_handle_yield_resp(const struct msgq_msg *msg)
{
	unsigned int next;

	spin_lock(&msgq_ctx.reply_lock);
	next = (msgq_ctx.reply_head + 1) % MSGQ_REPLY_SLOTS;
	if (next == msgq_ctx.reply_tail) {
		spin_unlock(&msgq_ctx.reply_lock);
		pr_err("reply queue is full\n");
		return;
	}
	memcpy(msgq_ctx.replies[msgq_ctx.reply_head].data, msg->data,
	       MSGQ_PAYLOAD_SIZE);
	msgq_ctx.reply_head = next;
	spin_unlock(&msgq_ctx.reply_lock);

	complete(&msgq_ctx.reply_done);
}

static void msgq_handle_fast_resp(const struct msgq_msg *msg)
{
	spin_lock(&msgq_ctx.fast_lock);
	if (msgq_ctx.fast_valid) {
		spin_unlock(&msgq_ctx.fast_lock);
		pr_err("unexpected fast call reply\n");
		return;
	}
	memcpy(msgq_ctx.fast_reply.data, msg->data, MSGQ_PAYLOAD_SIZE);
	msgq_ctx.fast_valid = true;
	spin_unlock(&msgq_ctx.fast_lock);

	complete(&msgq_ctx.fast_done);
}

static int msgq_recv_thread(void *data)
{
	struct msgq_msg *msg;
	size_t size;
	int rc;

	msg = kmalloc(GH_MSGQ_MAX_MSG_SIZE_BYTES, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	/* mitee_msgq_remove() kills us out of gh_msgq_recv() */
	allow_signal(SIGKILL);

	while (!kthread_should_stop()) {
		memset(msg, 0, GH_MSGQ_MAX_MSG_SIZE_BYTES);
		rc = gh_msgq_recv(msgq_ctx.msgq, msg, GH_MSGQ_MAX_MSG_SIZE_BYTES,
				  &size, 0);
		if (rc) {
			if (!signal_pending(current))
				dev_err(msgq_ctx.dev, "receive failed: %d\n", rc);
			continue;
		}
		if (size < sizeof(msg->hdr) || msg->hdr.magic != MSGQ_MAGIC ||
		    msg->hdr.len > size - sizeof(msg->hdr)) {
			pr_err("bad message\n");
			continue;
		}

		switch (msg->hdr.type) {
		case MSGQ_MSG_CTRL:
			msgq_handle_ctrl(msg);
			break;
		case MSGQ_MSG_YIELD_RESP:
		case MSGQ_MSG_FAST_RESP:
			if (msg->hdr.len != MSGQ_PAYLOAD_SIZE) {
				pr_err("bad reply size %u\n", msg->hdr.len);
				break;
			}
			if (msg->hdr.type == MSGQ_MSG_YIELD_RESP)
				msgq_handle_yield_resp(msg);
			else
				msgq_handle_fast_resp(msg);
			break;
		default:
			pr_err("unknown msg type %u\n", msg->hdr.type);
			break;
		}
	}

	kfree(msg);
	return 0;
}

static void msgq_args_to_payload(const union common_args *args, u64 *data)
{
	data[0] = args->in.cmd;
	memcpy(&data[1], args->in.param, sizeof(args->in.param));
}

static void msgq_payload_to_args(const u64 *data, union common_args *args)
{
	args->out.resp = data[0];
	memcpy(args->out.param, &data[1], sizeof(args->out.param));
}

static int msgq_yielding_call(union common_args *args)
{
	u64 data[5];
	int rc;

	msgq_args_to_payload(args, data);
	rc = msgq_send_request(MSGQ_MSG_YIELD_REQ, data);
	if (rc)
		return rc;

	spin_lock(&msgq_ctx.reply_lock);
	while (msgq_ctx.reply_head == msgq_ctx.reply_tail) {
		spin_unlock(&msgq_ctx.reply_lock);
		wait_for_completion(&msgq_ctx.reply_done);
		spin_lock(&msgq_ctx.reply_lock);
	}
	memcpy(data, msgq_ctx.replies[msgq_ctx.reply_tail].data, sizeof(data));
	msgq_ctx.reply_tail = (msgq_ctx.reply_tail + 1) % MSGQ_REPLY_SLOTS;
	spin_unlock(&msgq_ctx.reply_lock);

	msgq_payload_to_args(data, args);
	return 0;
}

static int msgq_fast_call(union common_args *args)
{
	u64 data[5];
	int rc;

	mutex_lock(&msgq_ctx.fast_mutex);

	msgq_args_to_payload(args, data);
	rc = msgq_send_request(MSGQ_MSG_FAST_REQ, data);
	if (rc)
		goto out;

	wait_for_completion(&msgq_ctx.fast_done);

	spin_lock(&msgq_ctx.fast_lock);
	if (msgq_ctx.fast_valid) {
		memcpy(data, msgq_ctx.fast_reply.data, sizeof(data));
		msgq_ctx.fast_valid = false;
	} else {
		rc = -EIO;
	}
	spin_unlock(&msgq_ctx.fast_lock);

	if (!rc)
		msgq_payload_to_args(data, args);
out:
	mutex_unlock(&msgq_ctx.fast_mutex);
	return rc;
}

static int mitee_msgq_call(union common_args *args)
{
	u64 cmd = args->in.cmd;

	/* buffer configuration is answered locally */
	switch (cmd) {
	case OPTEE_FFA_GET_SHM_CONFIG:
		args->out.resp = msgq_ctx.shm.start;
		args->out.param[0] = MSGQ_STATIC_SHM_SIZE;
		args->out.param[1] = 1;
		return 0;
	case OPTEE_FFA_GET_MITEE_LOG_BUFFER:
		args->out.resp = msgq_ctx.shm.start + MSGQ_STATIC_SHM_SIZE;
		args->out.param[0] = resource_size(&msgq_ctx.shm) -
				     MSGQ_STATIC_SHM_SIZE;
		return 0;
	case OPTEE_FFA_SET_MITEE_MSG_BUFFER:
		msgq_ctx.msg_buf_pa = args->in.param[0];
		msgq_ctx.msg_buf_size = args->in.param[1];
		return 0;
	}

	if (msgq_ctx.state < MSGQ_CONNECTED) {
		pr_err("msgq didn't set up, try later\n");
		return -EAGAIN;
	}

	if (cmd & BIT(OPTEE_FFA_YIELDING_CALL_BIT))
		return msgq_yielding_call(args);

	return msgq_fast_call(args);
}

/*
 * 6. Driver initialization
 */
static int msgq_rm_notifier(struct notifier_block *nb, unsigned long cmd,
			    void *data)
{
	struct gh_rm_notif_vm_status_payload *payload = data;
	gh_vmid_t peer_vmid;

	if (cmd != GH_RM_NOTIF_VM_STATUS)
		return NOTIFY_DONE;
	if (payload->vm_status != GH_RM_VM_STATUS_READY &&
	    payload->vm_status != GH_RM_VM_STATUS_RESET)
		return NOTIFY_DONE;
	if (ghd_rm_get_vmid(msgq_ctx.peer_name, &peer_vmid) ||
	    peer_vmid != payload->vmid)
		return NOTIFY_DONE;

	if (payload->vm_status == GH_RM_VM_STATUS_RESET) {
		msgq_ctx.state = MSGQ_DOWN;
		msgq_unshare_static();
		return NOTIFY_OK;
	}

	if (ghd_rm_get_vmid(GH_PRIMARY_VM, &msgq_ctx.self_vmid))
		return NOTIFY_DONE;
	msgq_ctx.peer_vmid = peer_vmid;
	msgq_ctx.state = MSGQ_VM_UP;

	if (msgq_share_contiguous(msgq_ctx.shm.start,
				  resource_size(&msgq_ctx.shm),
				  msgq_ctx.shm_label, &msgq_ctx.shm_handle)) {
		pr_err("failed to share the static buffer\n");
		return NOTIFY_OK;
	}
	msgq_ctx.shm_shared = true;

	mitee_rpc_register_callback(REE_CALLBACK_MODULE_TEE_FRAMEWORK,
				    OPTEE_REE_CALLBACK_ALLOCATE_SECMEM,
				    mitee_msgq_rpc_callback);
	mitee_rpc_register_callback(REE_CALLBACK_MODULE_TEE_FRAMEWORK,
				    OPTEE_REE_CALLBACK_FREE_SECMEM,
				    mitee_msgq_rpc_callback);
	return NOTIFY_OK;
}

static int mitee_msgq_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device_node *shm_np;
	int rc;

	if (!of_property_read_bool(np, "xiaomi,master"))
		return -EINVAL;

	rc = of_property_read_u32(np, "gunyah-shm-label", &msgq_ctx.shm_label);
	if (rc) {
		dev_err(&pdev->dev, "failed to read label info %d\n", rc);
		return rc;
	}

	rc = of_property_read_u32(np, "peer-name", &msgq_ctx.peer_name);
	if (rc)
		msgq_ctx.peer_name = GH_SELF_VM;

	shm_np = of_parse_phandle(np, "shared-buffer", 0);
	if (!shm_np) {
		dev_err(&pdev->dev, "no shared-buffer\n");
		return -EINVAL;
	}
	rc = of_address_to_resource(shm_np, 0, &msgq_ctx.shm);
	of_node_put(shm_np);
	if (rc)
		return rc;
	if (resource_size(&msgq_ctx.shm) <= MSGQ_STATIC_SHM_SIZE) {
		dev_err(&pdev->dev, "shared-buffer too small\n");
		return -EINVAL;
	}

	msgq_ctx.dev = &pdev->dev;
	msgq_ctx.state = MSGQ_DOWN;
	spin_lock_init(&msgq_ctx.reply_lock);
	init_completion(&msgq_ctx.reply_done);
	mutex_init(&msgq_ctx.fast_mutex);
	spin_lock_init(&msgq_ctx.fast_lock);
	init_completion(&msgq_ctx.fast_done);
	mutex_init(&msgq_ctx.shm_mutex);
	mutex_init(&msgq_ctx.secmem_mutex);
	INIT_LIST_HEAD(&msgq_ctx.secmem_list);

	rc = rhashtable_init(&msgq_ctx.shm_handles, &shm_rhash_params);
	if (rc)
		return rc;

	msgq_ctx.msgq = gh_msgq_register(GH_MSGQ_LABEL_TEST_OEMVM);
	if (IS_ERR(msgq_ctx.msgq)) {
		rc = PTR_ERR(msgq_ctx.msgq);
		goto err_destroy_rhash;
	}

	msgq_ctx.rm_nb.notifier_call = msgq_rm_notifier;
	msgq_ctx.rm_nb.priority = INT_MAX;
	rc = gh_rm_register_notifier(&msgq_ctx.rm_nb);
	if (rc)
		goto err_unregister_msgq;

	msgq_ctx.recv_task = kthread_run(msgq_recv_thread, NULL,
					 "mitee_msgq_recv");
	if (IS_ERR(msgq_ctx.recv_task)) {
		rc = PTR_ERR(msgq_ctx.recv_task);
		goto err_unregister_notifier;
	}

	return 0;

err_unregister_notifier:
	gh_rm_unregister_notifier(&msgq_ctx.rm_nb);
err_unregister_msgq:
	gh_msgq_unregister(msgq_ctx.msgq);
err_destroy_rhash:
	rhashtable_destroy(&msgq_ctx.shm_handles);
	return rc;
}

static void mitee_msgq_remove(struct platform_device *pdev)
{
	gh_rm_unregister_notifier(&msgq_ctx.rm_nb);

	/* gh_msgq_recv() sleeps interruptibly, stop it before the queue goes */
	send_sig(SIGKILL, msgq_ctx.recv_task, 1);
	kthread_stop(msgq_ctx.recv_task);
	gh_msgq_unregister(msgq_ctx.msgq);

	msgq_unshare_static();

	rhashtable_free_and_destroy(&msgq_ctx.shm_handles, rh_free_fn, NULL);
}

static const struct of_device_id mitee_msgq_match[] = {
	{ .compatible = "xiaomi,mitee-hlos-ipc" },
	{}
};
MODULE_DEVICE_TABLE(of, mitee_msgq_match);

static struct platform_driver mitee_msgq_driver = {
	.probe = mitee_msgq_probe,
	.remove = mitee_msgq_remove,
	.driver = {
		.name = "mitee_ipc",
		.of_match_table = mitee_msgq_match,
		.probe_type = PROBE_FORCE_SYNCHRONOUS,
	},
};

static int mitee_msgq_register(void)
{
	return platform_driver_register(&mitee_msgq_driver);
}

static void mitee_msgq_unregister(void)
{
	platform_driver_unregister(&mitee_msgq_driver);
}

const struct mitee_comm_ops mitee_msgq_comm_ops = {
	.init = mitee_msgq_register,
	.deinit = mitee_msgq_unregister,
	.call = mitee_msgq_call,
	.from_msg_param = from_msg_param_msgq_mem,
	.to_msg_param = to_msg_param_msgq_mem,
	.shm_register = optee_msgq_shm_register,
	.shm_unregister = optee_msgq_shm_unregister,
	.shm_unregister_supp = optee_msgq_shm_unregister_supp,
};
