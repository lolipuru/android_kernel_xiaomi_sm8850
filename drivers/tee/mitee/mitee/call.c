// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2015-2021, Linaro Limited
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": [%s %d] " fmt, __func__, __LINE__

#include <linux/dcache.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/thread_info.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/uuid.h>

#include <tee_drv.h>
#include "optee_bench.h"
#include "optee_private.h"
#include "tee_bench.h"

#define MAX_TPATH_LEN				512

void optee_cq_wait_init(struct optee_call_queue *cq,
			struct optee_call_waiter *w)
{
	/*
	 * We're preparing to make a call to secure world. In case we can't
	 * allocate a thread in secure world we'll end up waiting in
	 * optee_cq_wait_for_completion().
	 *
	 * Normally if there's no contention in secure world the call will
	 * complete and we can cleanup directly with optee_cq_wait_final().
	 */
	mutex_lock(&cq->mutex);

	/*
	 * We add ourselves to the queue, but we don't wait. This
	 * guarantees that we don't lose a completion if secure world
	 * returns busy and another thread just exited and try to complete
	 * someone.
	 */
	init_completion(&w->c);
	list_add_tail(&w->list_node, &cq->waiters);

	mutex_unlock(&cq->mutex);
}

void optee_cq_wait_for_completion(struct optee_call_queue *cq,
				  struct optee_call_waiter *w)
{
	wait_for_completion(&w->c);

	mutex_lock(&cq->mutex);

	/* Move to end of list to get out of the way for other waiters */
	list_del(&w->list_node);
	reinit_completion(&w->c);
	list_add_tail(&w->list_node, &cq->waiters);

	mutex_unlock(&cq->mutex);
}

static void optee_cq_complete_one(struct optee_call_queue *cq)
{
	struct optee_call_waiter *w = NULL;

	list_for_each_entry(w, &cq->waiters, list_node) {
		if ((w != NULL) && (!completion_done(&w->c))) {
			complete(&w->c);
			break;
		}
	}
}

void optee_cq_wait_final(struct optee_call_queue *cq,
			 struct optee_call_waiter *w)
{
	/*
	 * We're done with the call to secure world. The thread in secure
	 * world that was used for this call is now available for some
	 * other task to use.
	 */
	mutex_lock(&cq->mutex);

	/* Get out of the list */
	list_del(&w->list_node);

	/* Wake up one eventual waiting task */
	optee_cq_complete_one(cq);

	/*
	 * If we're completed we've got a completion from another task that
	 * was just done with its call to secure world. Since yet another
	 * thread now is available in secure world wake up another eventual
	 * waiting task.
	 */
	if (completion_done(&w->c))
		optee_cq_complete_one(cq);

	mutex_unlock(&cq->mutex);
}

/* Requires the filpstate mutex to be held */
static struct optee_session *find_session(struct optee_context_data *ctxdata,
					  u32 session_id)
{
	struct optee_session *sess;

	list_for_each_entry(sess, &ctxdata->sess_list, list_node)
		if (sess->session_id == session_id)
			return sess;
	pr_err("session is null\n");
	return NULL;
}

#if 0
struct tee_shm *optee_get_msg_arg(struct tee_context *ctx, size_t num_params,
								  struct optee_msg_arg **msg_arg)
{
	struct optee *optee = tee_get_drvdata(ctx->teedev);
	size_t sz = OPTEE_MSG_GET_ARG_SIZE(num_params);
	struct tee_shm *shm;
	struct optee_msg_arg *ma;

	/*
	 * rpc_arg_count is set to the number of allocated parameters in
	 * the RPC argument struct if a second MSG arg struct is expected.
	 * The second arg struct will then be used for RPC.
	 */
	if (optee->rpc_arg_count)
		sz += OPTEE_MSG_GET_ARG_SIZE(optee->rpc_arg_count);

	shm = tee_shm_alloc(ctx, sz, TEE_SHM_MAPPED | TEE_SHM_PRIV);
	if (IS_ERR(shm))
		return shm;

	ma = tee_shm_get_va(shm, 0);
	if (IS_ERR(ma)) {
		tee_shm_free(shm);
		return (void *)ma;
	}

	memset(ma, 0, OPTEE_MSG_GET_ARG_SIZE(num_params));
	ma->num_params = num_params;
	*msg_arg = ma;

	return shm;
}
#endif

/* only clients run from these directories may open sessions */
static const char * const mitee_ca_dirs[] = {
	"/vendor/bin",
	"/odm/bin",
	"/apex/com.google.android.widevine/bin/hw",
};

static bool ca_path_in_dir(const char *path, const char *dir)
{
	size_t len = strlen(dir);

	return !strncmp(path, dir, len) && (path[len] == '\0' || path[len] == '/');
}

static int optee_auth_ca(void)
{
	char tpath[MAX_TPATH_LEN];
	struct mm_struct *mm = current->mm;
	struct file *exe_file;
	char *path;
	int i, rc;

	/* in-kernel clients have no executable */
	if (!mm)
		return 0;

	rcu_read_lock();
	exe_file = rcu_dereference(mm->exe_file);
	path = exe_file ? d_path(&exe_file->f_path, tpath, sizeof(tpath)) :
			  NULL;
	rcu_read_unlock();
	if (!path)
		return 0;
	if (IS_ERR(path))
		return PTR_ERR(path);

	rc = -EACCES;
	for (i = 0; i < ARRAY_SIZE(mitee_ca_dirs); i++) {
		if (ca_path_in_dir(path, mitee_ca_dirs[i])) {
			rc = 0;
			break;
		}
	}
	if (rc)
		pr_err("fail to auth ca path: %s\n", path);

	return rc;
}

static int metadata_to_msg_param(const struct tee_ioctl_open_session_arg *arg,
				 struct optee_msg_arg *msg_arg)
{
	uuid_t client_uuid;
	int rc;

	msg_arg->cmd = OPTEE_MSG_CMD_OPEN_SESSION;
	msg_arg->cancel_id = arg->cancel_id;
	msg_arg->num_params = arg->num_params + 2;

	/*
	 * Initialize and add the meta parameters needed when opening a
	 * session.
	 */
	msg_arg->params[0].attr = OPTEE_MSG_ATTR_TYPE_VALUE_INPUT |
				  OPTEE_MSG_ATTR_META;
	msg_arg->params[1].attr = OPTEE_MSG_ATTR_TYPE_VALUE_INPUT |
				  OPTEE_MSG_ATTR_META;
	memcpy(&msg_arg->params[0].u.value, arg->uuid, sizeof(arg->uuid));
	msg_arg->params[1].u.value.c = arg->clnt_login;

	rc = tee_session_calc_client_uuid(&client_uuid, arg->clnt_login,
					  arg->clnt_uuid);
	if (rc) {
		pr_err("calc client uuid is failed\n");
		return rc;
	}
	export_uuid(msg_arg->params[1].u.octets, &client_uuid);

	return 0;
}

int optee_open_session(struct tee_context *ctx,
			   struct tee_ioctl_open_session_arg *arg,
			   struct tee_param *param)
{
	struct optee *optee = NULL;
	struct optee_context_data *ctxdata = NULL;
	int rc;
	struct optee_msg_arg *msg_arg = NULL;
	struct optee_session *sess = NULL;
	size_t sz;

	if (IS_ERR(ctx) || IS_ERR(arg) || IS_ERR(param))
		return -EINVAL;

	rc = optee_auth_ca();
	if (rc)
		return rc;

	optee = tee_get_drvdata(ctx->teedev);
	ctxdata = ctx->data;
	/* +2 for the meta parameters added below */
	sz = OPTEE_MSG_GET_ARG_SIZE(arg->num_params + 2);
	msg_arg = kzalloc(sz, GFP_KERNEL);
	if (!msg_arg) {
		pr_err("msg_arg alloc is failed\n");
		return -ENOMEM;
	}

	rc = metadata_to_msg_param(arg, msg_arg);
	if (rc) {
		pr_err("metadata to msg param is failed\n");
		goto out;
	}

	rc = optee->ops->to_msg_param(optee, msg_arg->params + 2,
				      arg->num_params, param);
	if (rc) {
		pr_err("to_msg_param is failed\n");
		goto out;
	}

	sess = kzalloc(sizeof(*sess), GFP_KERNEL);
	if (!sess) {
		rc = -ENOMEM;
		goto out;
	}

	if (optee->ops->do_call_with_arg(ctx, msg_arg)) {
		msg_arg->ret = TEEC_ERROR_COMMUNICATION;
		msg_arg->ret_origin = TEEC_ORIGIN_COMMS;
	}

	if (msg_arg->ret == TEEC_SUCCESS) {
		/* A new session has been created, add it to the list. */
		sess->session_id = msg_arg->session;
		mutex_lock(&ctxdata->mutex);
		list_add(&sess->list_node, &ctxdata->sess_list);
		mutex_unlock(&ctxdata->mutex);
	} else {
		pr_err("failed, msg_arg->ret: %#x\n",
			msg_arg->ret);
		kfree(sess);
	}

	if (optee->ops->from_msg_param(optee, param, arg->num_params,
				       msg_arg->params + 2)) {
		arg->ret = TEEC_ERROR_COMMUNICATION;
		arg->ret_origin = TEEC_ORIGIN_COMMS;
		/* Close session again to avoid leakage */
		optee_close_session(ctx, msg_arg->session);
		pr_err("from_msg_param is failed\n");
	} else {
		arg->session = msg_arg->session;
		arg->ret = msg_arg->ret;
		arg->ret_origin = msg_arg->ret_origin;
	}
	if (DEBUG_CALL)
		pr_info("session id = %d\n", arg->session);

	mitee_bench_add_node(arg->session, arg->uuid);
out:
	kfree(msg_arg);
	return rc;
}

int optee_close_session_helper(struct tee_context *ctx, u32 session)
{
	struct optee *optee = tee_get_drvdata(ctx->teedev);
	struct optee_msg_arg *msg_arg = NULL;
	size_t sz = OPTEE_MSG_GET_ARG_SIZE(0);

	msg_arg = kzalloc(sz, GFP_KERNEL);
	if (!msg_arg) {
		pr_err("msg_arg calloc is failed\n");
		return -ENOMEM;
	}

	msg_arg->num_params = 0;
	msg_arg->cmd = OPTEE_MSG_CMD_CLOSE_SESSION;
	msg_arg->session = session;
	optee->ops->do_call_with_arg(ctx, msg_arg);

	kfree(msg_arg);

	return 0;
}

int optee_close_session(struct tee_context *ctx, u32 session)
{
	struct optee_context_data *ctxdata = ctx->data;
	struct optee_session *sess;

	mitee_bench_del_node(session);
	/* Check that the session is valid and remove it from the list */
	mutex_lock(&ctxdata->mutex);
	sess = find_session(ctxdata, session);
	if (sess)
		list_del(&sess->list_node);
	mutex_unlock(&ctxdata->mutex);
	if (!sess) {
		pr_err("sess is not null\n");
		return -EINVAL;
	}
	kfree(sess);

	return optee_close_session_helper(ctx, session);
}

int optee_invoke_func(struct tee_context *ctx, struct tee_ioctl_invoke_arg *arg,
		      struct tee_param *param)
{
	struct optee *optee = tee_get_drvdata(ctx->teedev);
	struct optee_context_data *ctxdata = ctx->data;
	struct optee_msg_arg *msg_arg = NULL;
	struct optee_session *sess = NULL;
	size_t sz = OPTEE_MSG_GET_ARG_SIZE(arg->num_params);
	int rc;

	mitee_bench_timestamp(arg->session, "optee_invoke_func enter");

	/* Check that the session is valid */
	mutex_lock(&ctxdata->mutex);
	sess = find_session(ctxdata, arg->session);
	mutex_unlock(&ctxdata->mutex);
	if (!sess) {
		pr_err("sess is null\n");
		return -EINVAL;
	}

	msg_arg = kzalloc(sz, GFP_KERNEL);
	if (!msg_arg) {
		pr_err("msg_arg alloc is failed\n");
		return -ENOMEM;
	}
	msg_arg->num_params = arg->num_params;
	msg_arg->cmd = OPTEE_MSG_CMD_INVOKE_COMMAND;
	msg_arg->func = arg->func;
	msg_arg->session = arg->session;
	msg_arg->cancel_id = arg->cancel_id;
	rc = optee->ops->to_msg_param(optee, msg_arg->params, arg->num_params,
				      param);
	if (rc) {
		pr_err("to_msg_param is failed\n");
		goto out;
	}

	mitee_bench_timestamp(arg->session, "do_call_with_arg pre in");
	if (optee->ops->do_call_with_arg(ctx, msg_arg)) {
		msg_arg->ret = TEEC_ERROR_COMMUNICATION;
		msg_arg->ret_origin = TEEC_ORIGIN_COMMS;
		pr_err("do_call_with_arg is failed\n");
	}
	mitee_bench_timestamp(arg->session, "do_call_with_arg out");
	if (optee->ops->from_msg_param(optee, param, arg->num_params,
				       msg_arg->params)) {
		msg_arg->ret = TEEC_ERROR_COMMUNICATION;
		msg_arg->ret_origin = TEEC_ORIGIN_COMMS;
		pr_err("from_msg_param is failed\n");
	}

	arg->ret = msg_arg->ret;
	arg->ret_origin = msg_arg->ret_origin;
out:
	kfree(msg_arg);
	mitee_bench_timestamp(arg->session, "optee_invoke_func exit");
	return rc;
}

int optee_cancel_req(struct tee_context *ctx, u32 cancel_id, u32 session)
{
	pr_err("%s cancel is not support!\n", __func__);
	return -EACCES;

#if 0
	struct optee *optee = tee_get_drvdata(ctx->teedev);
	struct optee_context_data *ctxdata = ctx->data;
	struct tee_shm *shm;
	struct optee_msg_arg *msg_arg;
	struct optee_session *sess;

	/* Check that the session is valid */
	mutex_lock(&ctxdata->mutex);
	sess = find_session(ctxdata, session);
	mutex_unlock(&ctxdata->mutex);
	if (!sess)
		return -EINVAL;

	shm = optee_get_msg_arg(ctx, 0, &msg_arg);
	if (IS_ERR(shm))
		return PTR_ERR(shm);

	msg_arg->cmd = OPTEE_MSG_CMD_CANCEL;
	msg_arg->session = session;
	msg_arg->cancel_id = cancel_id;
	optee->ops->do_call_with_arg(ctx, shm);

	tee_shm_free(shm);
	return 0;
#endif
}

static bool is_normal_memory(pgprot_t p)
{
#if defined(CONFIG_ARM)
	return (((pgprot_val(p) & L_PTE_MT_MASK) == L_PTE_MT_WRITEALLOC) ||
		((pgprot_val(p) & L_PTE_MT_MASK) == L_PTE_MT_WRITEBACK));
#elif defined(CONFIG_ARM64)
	return (pgprot_val(p) & PTE_ATTRINDX_MASK) == PTE_ATTRINDX(MT_NORMAL);
#else
#error "Unuspported architecture"
#endif
}

static int __check_mem_type(struct mm_struct *mm, unsigned long start,
			    unsigned long end)
{
	struct vm_area_struct *vma;

	VMA_ITERATOR(vmi, mm, start);

	for_each_vma_range(vmi, vma, end) {
		if (!is_normal_memory(vma->vm_page_prot)) {
			pr_err("memory is error\n");
			return -EINVAL;
		}
	}

	return 0;
}

int optee_check_mem_type(unsigned long start, size_t num_pages)
{
	struct mm_struct *mm = current->mm;
	int rc;

	/*
	 * Allow kernel address to register with OP-TEE as kernel
	 * pages are configured as normal memory only.
	 */
	if (virt_addr_valid((const void *)(uintptr_t)start) ||
	    is_vmalloc_addr((const void *)(uintptr_t)start))
		return 0;

	mmap_read_lock(mm);
	rc = __check_mem_type(mm, start, start + num_pages * PAGE_SIZE);
	mmap_read_unlock(mm);

	return rc;
}
