// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 XiaoMi, Inc.
 * All Rights Reserved.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": tui: " fmt

#include <linux/anon_inodes.h>
#include <linux/fs.h>
#include <linux/idr.h>
#include <linux/mm.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/vmalloc.h>

#include "tui_shm.h"

/* buffers that can be looked up by id, each holds a reference */
static DEFINE_IDR(tui_shm_idr);
static DEFINE_MUTEX(tui_shm_mutex);

void tui_shm_deinit(void)
{
	struct tui_shm *shm;
	int id;

	mutex_lock(&tui_shm_mutex);
	idr_for_each_entry(&tui_shm_idr, shm, id) {
		idr_remove(&tui_shm_idr, id);
		tui_shm_put(shm);
	}
	mutex_unlock(&tui_shm_mutex);

	idr_destroy(&tui_shm_idr);
}

struct tui_shm *tui_shm_alloc(size_t size, const uint8_t *name)
{
	struct tui_shm *shm;
	int rc;

	if (!size)
		return ERR_PTR(-EINVAL);

	shm = kzalloc(sizeof(*shm), GFP_KERNEL);
	if (!shm)
		return ERR_PTR(-ENOMEM);

	shm->size = PAGE_ALIGN(size);
	shm->vaddr = vmalloc_user(shm->size);
	if (!shm->vaddr) {
		rc = -ENOMEM;
		goto err_free_shm;
	}
	strscpy(shm->name, name, sizeof(shm->name));
	refcount_set(&shm->refcount, 1);

	mutex_lock(&tui_shm_mutex);
	shm->id = idr_alloc(&tui_shm_idr, shm, 1, 0, GFP_KERNEL);
	mutex_unlock(&tui_shm_mutex);
	if (shm->id < 0) {
		rc = shm->id;
		goto err_free_buf;
	}

	return shm;

err_free_buf:
	vfree(shm->vaddr);
err_free_shm:
	kfree(shm);
	return ERR_PTR(rc);
}

void tui_shm_put(struct tui_shm *shm)
{
	if (!refcount_dec_and_test(&shm->refcount))
		return;

	vfree(shm->vaddr);
	kfree(shm);
}

/* drop the id, the buffer lives on while it is still referenced */
void tui_shm_remove(struct tui_shm *shm)
{
	bool removed = false;

	mutex_lock(&tui_shm_mutex);
	if (idr_find(&tui_shm_idr, shm->id) == shm) {
		idr_remove(&tui_shm_idr, shm->id);
		removed = true;
	}
	mutex_unlock(&tui_shm_mutex);

	if (removed)
		tui_shm_put(shm);
}

/* returns a referenced buffer, release it with tui_shm_put() */
struct tui_shm *tui_shm_get_by_id(int id)
{
	struct tui_shm *shm;

	mutex_lock(&tui_shm_mutex);
	shm = idr_find(&tui_shm_idr, id);
	if (shm)
		refcount_inc(&shm->refcount);
	mutex_unlock(&tui_shm_mutex);

	return shm ?: ERR_PTR(-EINVAL);
}

static int tui_shm_fop_release(struct inode *inode, struct file *filp)
{
	tui_shm_put(filp->private_data);
	return 0;
}

static int tui_shm_fop_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct tui_shm *shm = filp->private_data;

	if (vma->vm_pgoff + vma_pages(vma) > shm->size >> PAGE_SHIFT)
		return -EINVAL;

	return remap_vmalloc_range(vma, shm->vaddr, vma->vm_pgoff);
}

static const struct file_operations tui_shm_fops = {
	.owner = THIS_MODULE,
	.release = tui_shm_fop_release,
	.mmap = tui_shm_fop_mmap,
};

/* the file holds its own reference to @shm */
struct file *tui_shm_get_file(struct tui_shm *shm)
{
	struct file *file;

	refcount_inc(&shm->refcount);
	file = anon_inode_getfile(shm->name, &tui_shm_fops, shm, O_RDWR);
	if (IS_ERR(file))
		tui_shm_put(shm);

	return file;
}
