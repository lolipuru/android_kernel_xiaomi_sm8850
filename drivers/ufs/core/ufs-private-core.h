/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Xiaomi ABL appends initcall_blacklist=ufshcd_core_init, so the GKI UFS core in
 * vmlinux never registers ufs_device_wlun and sd never binds the data LUNs. Like
 * stock (mi_ufs), ufs-qcom.ko links a private copy of the GKI 6.12 core instead
 * (pristine sources: drivers/ufs/core from kernel-6.12/common).
 */
#ifndef _UFS_PRIVATE_CORE_H
#define _UFS_PRIVATE_CORE_H

/*
 * Core translation units only: vmlinux already exports every one of these
 * names, so the private copy must not export anything.
 */
#ifdef UFS_PRIVATE_CORE_NOEXPORT
#include <linux/export.h>
#undef EXPORT_SYMBOL
#undef EXPORT_SYMBOL_GPL
#define EXPORT_SYMBOL(sym)
#define EXPORT_SYMBOL_GPL(sym)
#endif

int ufshcd_private_core_init(void);
void ufshcd_private_core_exit(void);

#endif /* _UFS_PRIVATE_CORE_H */
