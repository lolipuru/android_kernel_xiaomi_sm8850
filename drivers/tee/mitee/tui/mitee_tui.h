/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2023 XiaoMi, Inc.
 * All Rights Reserved.
 */

#ifndef MITEE_TUI_H
#define MITEE_TUI_H

#include <linux/types.h>

/*
 * defines for the ioctl TUI driver module function call from user space.
 */
#define TUI_DEV_NAME	"mitee-tui"
#define TUI_SUPP_DEV_NAME	"mitee-tui-supp"

#define TUI_IO_MAGIC	't'

#define TUI_IO_NOTIFY	_IO(TUI_IO_MAGIC, 1)
#define TUI_IO_WAITCMD	_IO(TUI_IO_MAGIC, 2)
#define TUI_IO_ACK	_IO(TUI_IO_MAGIC, 3)
#define TUI_CLOSE_SESSION _IO(TUI_IO_MAGIC, 4)
#define TUI_IO_NOTIFY_CANCEL _IO(TUI_IO_MAGIC, 5)
#define TUI_IO_REGISTER_CALLBACK _IO(TUI_IO_MAGIC, 6)
#define TUI_IO_ALLOC_SHM _IO(TUI_IO_MAGIC, 7)
#define TUI_IO_FREE_SHM _IO(TUI_IO_MAGIC, 8)

//notify cancel macro
#define TUI_MAGIC_NUM 0x0a0b0c0d
#define TUI_SVC_ID 0x0
#define TUI_NOTIFY_CANCEL 0x0c

/**
 * Return codes of driver commands.
 */
#define TUI_OK                      0
#define TUI_ERR_UNKNOWN_CMD         1
#define TUI_ERR_NOT_SUPPORTED       2
#define TUI_ERR_INTERNAL_ERROR      3
#define TUI_ERR_NO_RESPONSE         4
#define TUI_ERR_BAD_PARAMETERS      5
#define TUI_ERR_NO_EVENT            6
#define TUI_ERR_OUT_OF_MEMORY       7
#define TUI_ERR_TIMEOUT             8

/* commands from the TUI TA, REE_CALLBACK_MODULE_TEE_TUI */
#define CMD_TUI_SVC_NONE             0
#define CMD_TUI_SVC_GET_SCREEN_INFO  1
#define CMD_TUI_SVC_OPEN_SESSION     2
#define CMD_TUI_SVC_CLOSE_SESSION    3
#define CMD_TUI_SVC_GET_FONT_BUFFER  4

/* Command header */
struct mitee_tui_command_t {
	uint32_t id;
};

/* Resolution */
struct mitee_tui_resolution_t {
	uint32_t width;
	uint32_t height;
	uint32_t dpi;
};

/* tui_shm construct */
struct mitee_tui_shm_t {
	uint8_t shm_name[16];
	int32_t shm_fd;
	int32_t shm_id;
	uint64_t shm_size;
};

/* Response header */
struct mitee_tui_response_t {
	uint32_t id;
	uint32_t return_code;
	union {
		struct mitee_tui_resolution_t resolution;
		struct mitee_tui_shm_t fontShm;
	} data;
};

/* CMD_TUI_SVC_GET_SCREEN_INFO reply to the TA */
struct screen_info_t {
	uint64_t width;
	uint64_t height;
	uint32_t panel_vendor;	/* unused, the TUI service drives the panel */
	uint64_t pa;		/* unused, the TUI service owns the frame buffer */
};

/* tui_ioctl_data */
struct tui_ioctl_buf_data {
	uint64_t buf_ptr;
	uint64_t buf_len;
};

/* Command IDs */
#define MITEE_TUI_CMD_NONE                0
#define MITEE_TUI_CMD_GET_RESOLUTION      1
#define MITEE_TUI_CMD_OPEN_SESSION        2
#define MITEE_TUI_CMD_CLOSE_SESSION       3
#define MITEE_TUI_CMD_GET_FONT_BUFFER     4

/* Return codes */
#define MITEE_TUI_OK                  0
#define MITEE_TUI_ERROR               1
#define MITEE_TUI_ERR_UNKNOWN_CMD     2

int mitee_tui_init(void);
void mitee_tui_exit(void);

#endif /* MITEE_TUI_H */
