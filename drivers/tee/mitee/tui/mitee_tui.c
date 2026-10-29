// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 XiaoMi, Inc.
 * All Rights Reserved.
 *
 * Trusted UI relay. The TUI TA asks for screen info, session changes and
 * the font through MiTEE RPC callbacks; they are forwarded to the TUI
 * service in user space, which drives the panel and touch itself.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": tui: " fmt

#include <linux/atomic.h>
#include <linux/cdev.h>
#include <linux/completion.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/jiffies.h>
#include <linux/minmax.h>
#include <linux/poll.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

#include "../mitee/mitee_smc_notify.h"
#include "../mitee/optee_msg.h"
#include "../mitee/rpc_callback.h"
#include "mitee_tui.h"
#include "tui_shm.h"

#define TUI_RESPONSE_TIMEOUT_MS	5000

/* command for the TUI service and its response */
static struct mitee_tui_command_t tui_cmd;
static struct mitee_tui_response_t tui_rsp;
static DECLARE_COMPLETION(tui_rsp_done);

/* a command is pending, reported through poll() */
static DECLARE_WAIT_QUEUE_HEAD(tui_event_wq);
static atomic_t tui_event = ATOMIC_INIT(0);
static atomic_t tui_opened = ATOMIC_INIT(0);

/* notification token for cancelling a TUI session from the REE */
static int tui_token;

/* font being copied to the TA, chunk by chunk */
static struct {
	struct tui_shm *shm;
	u64 size;
} tui_font;

static const struct class tui_class = {
	.name = "mitee_tui",
};

static dev_t tui_devt;
static struct cdev tui_cdev;

static uint32_t send_cmd_to_user(uint32_t command_id)
{
	uint32_t ret;

	if (!atomic_read(&tui_opened)) {
		pr_err("no TUI service for command %u\n", command_id);
		return TUI_ERR_INTERNAL_ERROR;
	}

	reinit_completion(&tui_rsp_done);
	tui_cmd.id = command_id;

	/* let the service know (poll) that there is a command to process */
	atomic_set(&tui_event, 1);
	wake_up_interruptible(&tui_event_wq);

	if (!wait_for_completion_timeout(&tui_rsp_done,
			msecs_to_jiffies(TUI_RESPONSE_TIMEOUT_MS))) {
		pr_err("no response to command %u\n", command_id);
		ret = TUI_ERR_TIMEOUT;
		goto out;
	}

	if (tui_rsp.id != command_id) {
		pr_err("wrong response id %u to command %u\n", tui_rsp.id,
		       command_id);
		ret = TUI_ERR_INTERNAL_ERROR;
		goto out;
	}

	switch (tui_rsp.return_code) {
	case MITEE_TUI_OK:
		ret = TUI_OK;
		break;
	case MITEE_TUI_ERR_UNKNOWN_CMD:
		ret = TUI_ERR_UNKNOWN_CMD;
		break;
	default:
		ret = TUI_ERR_INTERNAL_ERROR;
		break;
	}

out:
	tui_cmd.id = MITEE_TUI_CMD_NONE;
	return ret;
}

static void tui_font_release(void)
{
	if (!tui_font.shm)
		return;

	/* the font buffer is consumed, the service doesn't free it itself */
	tui_shm_remove(tui_font.shm);
	tui_shm_put(tui_font.shm);
	tui_font.shm = NULL;
	tui_font.size = 0;
}

/*
 * The TA reads the font in chunks: value->c is the offset and value->b the
 * chunk size. The first chunk replies with the font size in value->b, the
 * others with the number of bytes copied.
 */
static uint32_t tui_get_font(struct optee_msg_param_value *value, void *buf,
			     uint32_t size_in, uint32_t *size_out)
{
	u64 offset = value->c;
	struct tui_shm *shm;
	uint32_t ret;
	u64 size;

	if (offset == 0) {
		tui_font_release();

		ret = send_cmd_to_user(MITEE_TUI_CMD_GET_FONT_BUFFER);
		if (ret != TUI_OK)
			return ret;

		shm = tui_shm_get_by_id(tui_rsp.data.fontShm.shm_id);
		if (IS_ERR(shm)) {
			pr_err("no font buffer %d\n",
			       tui_rsp.data.fontShm.shm_id);
			return TUI_ERR_BAD_PARAMETERS;
		}
		if (tui_rsp.data.fontShm.shm_size > shm->size) {
			pr_err("font size %llu over buffer size %zu\n",
			       tui_rsp.data.fontShm.shm_size, shm->size);
			tui_shm_put(shm);
			return TUI_ERR_BAD_PARAMETERS;
		}
		tui_font.shm = shm;
		tui_font.size = tui_rsp.data.fontShm.shm_size;
	}

	if (!tui_font.shm || offset > tui_font.size)
		return TUI_ERR_BAD_PARAMETERS;

	size = min3(value->b, (u64)size_in, tui_font.size - offset);
	memcpy(buf, tui_font.shm->vaddr + offset, size);

	value->b = offset ? size : tui_font.size;
	*size_out = size;

	if (offset + size >= tui_font.size)
		tui_font_release();

	return TUI_OK;
}

static uint32_t tui_process_tuisvc_callback(struct optee_msg_param_value *value,
					    void *buf, uint32_t size_in,
					    uint32_t *size_out)
{
	uint32_t command_id = value->a;
	struct screen_info_t *info = buf;
	uint32_t ret;

	switch (command_id) {
	case CMD_TUI_SVC_GET_SCREEN_INFO:
		if (!buf || size_in < sizeof(*info))
			return TUI_ERR_BAD_PARAMETERS;

		ret = send_cmd_to_user(MITEE_TUI_CMD_GET_RESOLUTION);
		if (ret != TUI_OK)
			break;

		memset(info, 0, sizeof(*info));
		info->width = tui_rsp.data.resolution.width;
		info->height = tui_rsp.data.resolution.height;
		*size_out = sizeof(*info);
		break;
	case CMD_TUI_SVC_OPEN_SESSION:
		ret = send_cmd_to_user(MITEE_TUI_CMD_OPEN_SESSION);
		break;
	case CMD_TUI_SVC_CLOSE_SESSION:
		ret = send_cmd_to_user(MITEE_TUI_CMD_CLOSE_SESSION);
		if (ret == TUI_OK)
			value->a = 0;
		break;
	case CMD_TUI_SVC_GET_FONT_BUFFER:
		if (!buf)
			return TUI_ERR_BAD_PARAMETERS;
		ret = tui_get_font(value, buf, size_in, size_out);
		break;
	default:
		pr_err("unknown command %u\n", command_id);
		ret = TUI_ERR_UNKNOWN_CMD;
		break;
	}

	return ret;
}

/* a connect failure leaves an errno in place of the token */
static bool tui_token_valid(void)
{
	return tui_token && !IS_ERR_VALUE((long)tui_token);
}

static int tui_notify_cancel(void)
{
	int rc;

	if (!tui_token_valid())
		tui_token = mitee_smc_notify_connect(TUI_SVC_ID, TUI_MAGIC_NUM);
	if (!tui_token_valid()) {
		pr_err("notify connect failed: %d\n", tui_token);
		return -EIO;
	}

	rc = mitee_smc_notify_signal(TUI_SVC_ID, tui_token, TUI_NOTIFY_CANCEL);
	if (rc)
		pr_err("notify signal failed: %d\n", rc);

	return 0;
}

static int tui_ioctl_ack(void __user *uarg)
{
	struct tui_ioctl_buf_data data;

	if (copy_from_user(&data, uarg, sizeof(data)))
		return -EFAULT;
	if (data.buf_len > sizeof(tui_rsp))
		return -EINVAL;
	if (copy_from_user(&tui_rsp, u64_to_user_ptr(data.buf_ptr),
			   data.buf_len))
		return -EFAULT;

	complete(&tui_rsp_done);
	return 0;
}

static int tui_ioctl_tui_shm_alloc(void __user *uarg)
{
	struct tui_ioctl_alloc_shm_data data;
	struct tui_shm *shm;
	struct file *file;
	int fd;

	if (copy_from_user(&data, uarg, sizeof(data)))
		return -EFAULT;
	if (data.flags)
		return -EINVAL;

	data.name[TUI_SHM_NAME_LENTH - 1] = '\0';
	if (!data.name[0])
		return -EINVAL;

	shm = tui_shm_alloc(data.size, data.name);
	if (IS_ERR(shm))
		return PTR_ERR(shm);

	fd = get_unused_fd_flags(0);
	if (fd < 0)
		goto err_remove_shm;

	file = tui_shm_get_file(shm);
	if (IS_ERR(file)) {
		put_unused_fd(fd);
		fd = PTR_ERR(file);
		goto err_remove_shm;
	}

	data.id = shm->id;
	data.fd = fd;
	if (copy_to_user(uarg, &data, sizeof(data))) {
		fput(file);
		put_unused_fd(fd);
		tui_shm_remove(shm);
		return -EFAULT;
	}

	fd_install(fd, file);
	return 0;

err_remove_shm:
	tui_shm_remove(shm);
	return fd;
}

static int tui_ioctl_tui_shm_free(void __user *uarg)
{
	struct tui_ioctl_alloc_shm_data data;
	struct tui_shm *shm;

	if (copy_from_user(&data, uarg, sizeof(data)))
		return -EFAULT;
	if (data.flags)
		return -EINVAL;

	shm = tui_shm_get_by_id(data.id);
	if (IS_ERR(shm))
		return PTR_ERR(shm);

	tui_shm_remove(shm);
	tui_shm_put(shm);
	return 0;
}

static long tui_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	void __user *uarg = (void __user *)arg;

	switch (cmd) {
	case TUI_IO_NOTIFY:
	case TUI_CLOSE_SESSION:
		return 0;
	case TUI_IO_WAITCMD:
		if (copy_to_user(uarg, &tui_cmd, sizeof(tui_cmd)))
			return -EFAULT;
		return 0;
	case TUI_IO_ACK:
		return tui_ioctl_ack(uarg);
	case TUI_IO_NOTIFY_CANCEL:
		return tui_notify_cancel();
	case TUI_IO_REGISTER_CALLBACK:
		mitee_rpc_register_callback(REE_CALLBACK_MODULE_TEE_TUI,
					    OPTEE_REE_CALLBACK_CALL,
					    tui_process_tuisvc_callback);
		return 0;
	case TUI_IO_ALLOC_SHM:
		return tui_ioctl_tui_shm_alloc(uarg);
	case TUI_IO_FREE_SHM:
		return tui_ioctl_tui_shm_free(uarg);
	default:
		return -ENOTTY;
	}
}

static __poll_t tui_poll(struct file *file, poll_table *wait)
{
	poll_wait(file, &tui_event_wq, wait);

	if (atomic_xchg(&tui_event, 0))
		return EPOLLIN | EPOLLRDNORM;

	return 0;
}

static int tui_open(struct inode *inode, struct file *file)
{
	atomic_inc(&tui_opened);
	return 0;
}

static int tui_release(struct inode *inode, struct file *file)
{
	atomic_dec(&tui_opened);
	return 0;
}

static const struct file_operations tui_fops = {
	.owner = THIS_MODULE,
	.poll = tui_poll,
	.unlocked_ioctl = tui_ioctl,
	.compat_ioctl = compat_ptr_ioctl,
	.open = tui_open,
	.release = tui_release,
};

/* the TUI service uses both nodes the same way */
static const char * const tui_dev_names[] = {
	TUI_DEV_NAME,
	TUI_SUPP_DEV_NAME,
};

int mitee_tui_init(void)
{
	struct device *dev;
	int rc, i;

	rc = alloc_chrdev_region(&tui_devt, 0, ARRAY_SIZE(tui_dev_names),
				 TUI_DEV_NAME);
	if (rc)
		return rc;

	cdev_init(&tui_cdev, &tui_fops);
	rc = cdev_add(&tui_cdev, tui_devt, ARRAY_SIZE(tui_dev_names));
	if (rc)
		goto err_unregister_region;

	rc = class_register(&tui_class);
	if (rc)
		goto err_cdev_del;

	for (i = 0; i < ARRAY_SIZE(tui_dev_names); i++) {
		dev = device_create(&tui_class, NULL, tui_devt + i, NULL, "%s",
				    tui_dev_names[i]);
		if (IS_ERR(dev)) {
			rc = PTR_ERR(dev);
			goto err_destroy_devices;
		}
	}

	return 0;

err_destroy_devices:
	while (i--)
		device_destroy(&tui_class, tui_devt + i);
	class_unregister(&tui_class);
err_cdev_del:
	cdev_del(&tui_cdev);
err_unregister_region:
	unregister_chrdev_region(tui_devt, ARRAY_SIZE(tui_dev_names));
	return rc;
}

void mitee_tui_exit(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(tui_dev_names); i++)
		device_destroy(&tui_class, tui_devt + i);
	class_unregister(&tui_class);
	cdev_del(&tui_cdev);
	unregister_chrdev_region(tui_devt, ARRAY_SIZE(tui_dev_names));

	tui_font_release();
	tui_shm_deinit();
}
