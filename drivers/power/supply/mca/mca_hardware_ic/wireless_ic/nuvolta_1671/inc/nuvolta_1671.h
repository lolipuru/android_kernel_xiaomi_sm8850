/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __NUVOLTA_1671_HEADER__
#define __NUVOLTA_1671_HEADER__

#include <linux/notifier.h>
#include <linux/reboot.h>

#define NU1671_REG_CMD			0x0000
#define NU1671_REG_CMD_LEN		0x0001
#define NU1671_REG_CMD_DATA(n)		(0x0002 + (n))
#define NU1671_REG_INT_L		0x0020
#define NU1671_REG_INT_H		0x0021
#define NU1671_REG_CHECK_DATA		0x0028
#define NU1671_REG_RTX_MODE		0x002f
#define NU1671_REG_TRIGGER		0x0060
#define NU1671_REG_READ_REQ		0x0062
#define NU1671_REG_TRX_CTRL		0x0063

#define NU1671_RTX_MODE_TRX		0x03
#define NU1671_READ_REQ_START		0x88
#define NU1671_TRX_CTRL_FW_CHECK	0x02
#define NU1671_TRX_CTRL_SLEEP		0x03

#define NU1671_REG_IC_FW_VER		0x2324
#define NU1671_REG_POWER_MODE		0x232a
#define NU1671_REG_SLEEP_CTRL		0x232c
#define NU1671_REG_TX_ID_L		0x232e
#define NU1671_REG_TX_ID_H		0x232f
#define NU1671_REG_CEP			0x2330
#define NU1671_REG_TX_TYPE		0x2339
#define NU1671_REG_POWEROFF_DUMP	0x2350
#define NU1671_REG_VOUT			0x2354
#define NU1671_REG_IOUT			0x2356
#define NU1671_REG_VRECT		0x2358
#define NU1671_REG_TEMP			0x235e
#define NU1671_REG_FREQ			0x2360
#define NU1671_REG_TRANS_RCV_HDR	0x237b
#define NU1671_REG_TRANS_RCV_DATA	0x237c
#define NU1671_REG_FACTORY_RCV		0x2394
#define NU1671_REG_SS_VOLTAGE		0x23b0
#define NU1671_REG_AUTH_DATA		0x23c0
#define NU1671_REG_FC_RESULT		0x23c2
#define NU1671_REG_ADAPTER_TYPE		0x23c3
#define NU1671_REG_UUID			0x23c4
#define NU1671_REG_TX_MAX_POWER		0x23d0

#define NU1671_SLEEP_CTRL_MAGIC		0x88
#define NU1671_POWEROFF_DUMP_LEN	16
#define NU1671_FACTORY_RCV_LEN		16
#define NU1671_FACTORY_RCV_MAX		15
#define NU1671_FW_CHECK_LEN		7
#define NU1671_FW_CHECK_CRC_OK		0x66

#define NU1671_CMD_ENABLE_REVERSE_FOD	0x23
#define NU1671_CMD_SET_VOUT		0x31
#define NU1671_CMD_CLEAR_INT		0x68
#define NU1671_CMD_TRANSMIT_PACKET	0x69
#define NU1671_CMD_TX_CUST		0x6a
#define NU1671_CMD_FOD_SET		0x98
#define NU1671_CMD_RENEGO		0xa8

#define NU1671_TRANS_DATA_3BYTE		0x38
#define NU1671_FASTCHG_SET_TX_VOLTAGE	0x0a
#define NU1671_ADAPTER_VOL_PACKET_LEN	0x05
#define NU1671_ADAPTER_VOL_SET_TYPE	0x02
#define NU1671_ADAPTER_VOL_TRIGGER	0x07
#define NU1671_VOUT_PACKET_LEN		0x02
#define NU1671_VOUT_TRIGGER		0x04
#define NU1671_CLEAR_INT_LEN		0x02
#define NU1671_CLEAR_INT_TRIGGER	0x04
#define NU1671_RENEGO_LEN		0x01
#define NU1671_RENEGO_TRIGGER		0x03
#define NU1671_TX_CUST_TRIGGER		0x01
#define NU1671_REV_FOD_DEFAULT_GAIN	94
#define NU1671_REV_FOD_EN_TRIGGER	0x04
#define NU1671_REV_FOD_DIS_TRIGGER	0x02

#define NU1671_FOD_CHUNK		5
#define NU1671_FOD_HDR_LEN		3
#define NU1671_FOD_DATA_REG		NU1671_REG_CMD_DATA(3)
#define NU1671_BPP_PLUS_FOD_CMD_LEN	0x0d
#define NU1671_BPP_PLUS_FOD_BYTES	10
#define NU1671_BPP_PLUS_FOD_TRIGGER	0x0f
#define NU1671_FOD_SET_DELAY_MS		20

#define NU1671_VOUT_SET_MAX_MV		19500
#define NU1671_VOUT_SET_MIN_MV		4000
#define NU1671_VOUT_SET_DEFAULT_MV	6000
#define NU1671_ADAPTER_VOL_MAX_MV	30000
#define NU1671_ADAPTER_VOL_MIN_MV	4000
#define NU1671_ADAPTER_VOL_DEFAULT_MV	6000
#define NU1671_CEP_MAX			1
#define NU1671_RX_OFFSET_THRESHOLD	5000
#define NU1671_TX_FAN_SPEED_MAX		10

#define NU1671_ADAPTER_QC3		6
#define NU1671_ADAPTER_PD		7
#define NU1671_ADAPTER_AUTH_FAILED	8
#define NU1671_ADAPTER_XIAOMI_QC3	9

#define NU1671_AUTH_UUID_VALID		5

#define NU1671_CP_MODE_2_1		2

#define FACTORY_TEST_CMD_ADAPTER_TYPE	0x0b
#define FACTORY_TEST_CMD_RX_IOUT	0x12
#define FACTORY_TEST_CMD_RX_VOUT	0x13
#define FACTORY_TEST_CMD_RX_CHIP_ID	0x23
#define FACTORY_TEST_CMD_RX_FW_ID	0x24
#define FACTORY_TEST_CMD_REVERSE_REQ	0x30
#define NU1671_CHIP_ID_H		0x16
#define NU1671_CHIP_ID_L		0x71

#define NU1671_REG_MTP_START_ADDR_L	0x0010
#define NU1671_REG_MTP_START_ADDR_H	0x0011
#define NU1671_REG_MTP_SECTOR		0x0012
#define NU1671_REG_MTP_CTRL0		0x0017
#define NU1671_REG_MTP_CTRL_MODE	0x0018
#define NU1671_REG_MTP_CTRL1		0x0019
#define NU1671_REG_MTP_CTRL2		0x001a
#define NU1671_REG_MTP_STATUS		0x001b
#define NU1671_REG_MTP_DATA(n)		(0x001c + (n))
#define NU1671_REG_MTP_BUSY		0x00a4
#define NU1671_REG_MTP_CFG		0x008c
#define NU1671_REG_MTP_SLEEP		0x0090
#define NU1671_REG_MCU_CTRL		0x1002
#define NU1671_REG_DTM_UNLOCK0		0x1f23
#define NU1671_REG_DTM_UNLOCK1		0x1f26
#define NU1671_REG_MTP_BUF0		0x2800
#define NU1671_REG_MTP_BUF1		0x3000
#define NU1671_REG_ANA_4918		0x4918
#define NU1671_REG_ANA_4A28		0x4a28
#define NU1671_REG_ANA_4A30		0x4a30
#define NU1671_REG_ANA_4A32		0x4a32
#define NU1671_REG_ANA_4A33		0x4a33
#define NU1671_REG_ANA_4A43		0x4a43
#define NU1671_REG_RST_5000		0x5000
#define NU1671_REG_RST_5008		0x5008

#define NU1671_MTP_STATUS_BUSY		0x80
#define NU1671_MTP_STATUS_ERR		0x40
#define NU1671_MTP_STATUS_RETRY		250
#define NU1671_MTP_BUSY_RETRY		50
#define NU1671_MTP_BURST		64
#define NU1671_FW_SIZE			32768
#define NU1671_FW_VERSION_OFFSET	0x7ff7

#define RX_CHECK_SUCCESS		(1 << 0)
#define TX_CHECK_SUCCESS		(1 << 1)
#define BOOT_CHECK_SUCCESS		(1 << 2)

#define DECL_INTERRUPT_MAP(regval, redir_irq)	\
	{					\
		.irq_regval = regval,		\
		.irq_flag = redir_irq,		\
	}

struct int_map_t {
	u32 irq_regval;
	int irq_flag;
};

enum rx_int_flag {
	RX_INT_UNKNOWN,
	RX_INT_LDO_ON,
	RX_INT_FAST_CHARGE,
	RX_INT_AUTHEN_FINISH,
	RX_INT_RENEGO_DONE,
	RX_INT_ALARM_SUCCESS,
	RX_INT_ALARM_FAIL,
	RX_INT_OOB_GOOD,
	RX_INT_RPP,
	RX_INT_TRANSPARENT_SUCCESS,
	RX_INT_TRANSPARENT_FAIL,
	RX_INT_FACTORY_TEST,
	RX_INT_OCP_OTP_ALARM,
	RX_INT_FIRST_AUTHEN,
	RX_INT_POWER_OFF,
	RX_INT_POWER_ON,
	RX_INT_RENEGO_FAIL,
	RX_INT_ERR_CODE,
};

enum rtx_int_flag {
	RTX_INT_UNKNOWN,
	RTX_INT_PING,
	RTX_INT_GET_RX,
	RTX_INT_CEP_TIMEOUT,
	RTX_INT_EPT,
	RTX_INT_PROTECTION,
	RTX_INT_GET_TX,
	RTX_INT_REVERSE_TEST_READY,
	RTX_INT_REVERSE_TEST_DONE,
	RTX_INT_FOD,
	RTX_INT_EPT_PKT,
	RTX_INT_ERR_CODE,
	RTX_INT_TX_DET_RX,
	RTX_INT_TX_CHS_UPDATE,
	RTX_INT_TX_BLE_CONNECT,
};

#define FOD_PARA_MAX_GROUP 22
enum fod_para_ele {
	FOD_PARA_TYPE = 0,
	FOD_PARA_LENGTH,
	FOD_PARA_UUID,
	FOD_PARA_PARAMS,
	FOD_PARA_PARAMS_MAG,
	FOD_PARA_PARAMS_MAG_THICK,
	FOD_PARA_MAX,
};

enum fod_params_set {
	FOD_PARAMS_SET_NORMAL = 0,
	FOD_PARAMS_SET_MAG,
	FOD_PARAMS_SET_MAG_THICK,
	FOD_PARAMS_SET_MAX,
};

#define DEFAULT_FOD_PARAM_LEN 20
enum PARAMS_T {
	PARAMS_T_GAIN,
	PARAMS_T_OFFSET,
	PARAMS_T_MAX,
};

enum wls_work_mode {
	RX_MODE,
	RTX_MODE,
};

struct wls_fw_parameters {
	u8 fw_rx_id;
	u8 fw_tx_id;
	u8 fw_boot_id;
};

struct params_t {
	u8 gain;
	u8 offset;
};

struct fod_params_t {
	u8 params_nums;
	u8 type;
	u8 length;
	int uuid;
	struct params_t params[FOD_PARAMS_SET_MAX][DEFAULT_FOD_PARAM_LEN];
};

struct nuvolta_1671_chg_proc_data {
	u8 epp;
	u8 fc_flag;
	int tx_speed;
	u16 adapter_type;
	u8 uuid[4];
	u8 epp_tx_id_h;
	u8 epp_tx_id_l;
	int int_flag;
	int int_trx_mode;
	bool is_car_tx;
	bool power_good_flag;
	bool reverse_chg_en;
	bool parallel_charge;
	int vout_setted;
	int ss_voltage;
	u8 tx_type;
	int cp_mode;
};

struct nuvolta_1671_chg {
	struct i2c_client *client;
	struct device *dev;
	struct regmap *regmap;
	int irq_gpio;
	int irq;
	int power_good_gpio;
	int power_good_irq;
	int enable_gpio;
	int hall_n_int_gpio;
	int hall_n_int_irq;
	int hall_s_int_gpio;
	int hall_s_int_irq;
	struct pinctrl *nu1671_pinctrl;
	unsigned char fw_bin[NU1671_FW_SIZE];
	int fw_bin_length;

	int project_vendor;
	int support_hall;
	int rx_role;
	int fw_version_index;

	struct wls_fw_parameters *wls_fw_data;
	int fw_data_size;
	const unsigned char *fw_data_ptr;
	int fod_params_size;
	int fod_params_2_1_size;
	struct fod_params_t fod_params[FOD_PARA_MAX_GROUP];
	struct fod_params_t fod_params_2_1[FOD_PARA_MAX_GROUP];
	struct fod_params_t fod_params_default;
	struct fod_params_t fod_params_bpp_plus;

	u8 wls_debug_one_fod_index;
	int wls_debug_set_fod_type;
	struct params_t wls_debug_one_fod_param;
	struct fod_params_t *wls_debug_all_fod_params;

	struct mutex i2c_lock;
	struct mutex wireless_chg_int_lock;
	struct mutex wireless_enable_lock;
	struct mutex data_transfer_lock;

	unsigned int disable_votes;
	bool enable_status;

	struct notifier_block shutdown_notifier;
	struct delayed_work init_detect_work;
	struct delayed_work pg_detect_work;
	struct delayed_work interrupt_work;
	struct delayed_work hall_interrupt_work;

	struct nuvolta_1671_chg_proc_data proc_data;

	bool magnetic_case_flag;
	bool hall_n_status;
	bool hall_s_status;
	bool hall_gpio_status;
	int fake_rx_offset;
	bool shutdown_flag;
};

#endif /* __NUVOLTA_1671_HEADER__ */
