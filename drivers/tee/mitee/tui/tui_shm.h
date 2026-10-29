/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2023 XiaoMi, Inc.
 * All Rights Reserved.
 */

#ifndef __TUI_SHM_H__
#define __TUI_SHM_H__

#include <linux/refcount.h>
#include <linux/types.h>

#define TUI_SHM_NAME_LENTH     16

struct file;

/* TUI_IO_ALLOC_SHM and TUI_IO_FREE_SHM argument */
struct tui_ioctl_alloc_shm_data {
	uint8_t name[TUI_SHM_NAME_LENTH];
	uint64_t size;
	uint32_t flags;	/* must be 0 */
	int32_t id;
	int32_t fd;
};

/*
 * Buffer shared between the TUI service (through an mmap()ed fd) and the
 * kernel (through its id). It is freed once both have let go of it.
 */
struct tui_shm {
	uint8_t name[TUI_SHM_NAME_LENTH];
	int id;
	void *vaddr;
	size_t size;
	refcount_t refcount;
};

void tui_shm_deinit(void);
struct tui_shm *tui_shm_alloc(size_t size, const uint8_t *name);
struct file *tui_shm_get_file(struct tui_shm *shm);
struct tui_shm *tui_shm_get_by_id(int id);
void tui_shm_remove(struct tui_shm *shm);
void tui_shm_put(struct tui_shm *shm);

#endif /* __TUI_SHM_H__ */
