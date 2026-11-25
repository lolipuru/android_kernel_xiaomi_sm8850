// SPDX-License-Identifier: GPL-2.0

#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/irqreturn.h>
#include <linux/regmap.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/pinctrl/consumer.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#include "inc/nuvolta_1671.h"
#include "inc/fw_update_nu1671.h"
#include <mca/common/mca_log.h>
#include <mca/common/mca_event.h>
#include <mca/common/mca_hwid.h>
#include <mca/common/mca_parse_dts.h>
#include <mca/platform/platform_wireless_class.h>
#include <mca/strategy/strategy_class.h>

#ifndef MCA_LOG_TAG
#define MCA_LOG_TAG "nuvolta_1671"
#endif

#define I2C_RETRY_CNT 3

#define NU1671_EN_VOTE_SELF BIT(0)
#define NU1671_EN_VOTE_MCA BIT(1)

static unsigned char fw_arrange_data[NU1671_FW_SIZE];

static const struct regmap_config nuvolta_1671_regmap_config = {
	.reg_bits = 16,
	.val_bits = 8,
	.max_register = 0xFFFF,
};

static int rx1671_read(struct nuvolta_1671_chg *chip, u8 *val, u16 addr)
{
	unsigned int temp = 0;
	int ret = 0, i;

	mutex_lock(&chip->i2c_lock);
	if (chip->shutdown_flag) {
		mca_log_err("failed-read, device is shutdown\n");
		goto out;
	}

	for (i = 0; i < I2C_RETRY_CNT; i++) {
		ret = regmap_read(chip->regmap, addr, &temp);
		if (ret >= 0) {
			*val = (u8)temp;
			break;
		}
		mca_log_err("failed-read, reg(0x%02X), ret(%d)\n", addr, ret);
	}
out:
	mutex_unlock(&chip->i2c_lock);
	return ret;
}

static int rx1671_write(struct nuvolta_1671_chg *chip, u8 val, u16 addr)
{
	int ret = 0, i;

	mutex_lock(&chip->i2c_lock);
	if (chip->shutdown_flag) {
		mca_log_err("failed-write, device is shutdown\n");
		goto out;
	}

	for (i = 0; i < I2C_RETRY_CNT; i++) {
		ret = regmap_write(chip->regmap, addr, val);
		if (ret >= 0)
			break;
		mca_log_err("failed-write, reg(0x%02X), ret(%d)\n", addr, ret);
	}
out:
	mutex_unlock(&chip->i2c_lock);
	return ret;
}

static int rx1671_read_buffer(struct nuvolta_1671_chg *chip, u8 *buf, u16 addr,
			      int size)
{
	int ret;

	mutex_lock(&chip->i2c_lock);
	if (chip->shutdown_flag) {
		mca_log_err("failed-bulk-read, device is shutdown\n");
		mutex_unlock(&chip->i2c_lock);
		return 0;
	}
	ret = regmap_bulk_read(chip->regmap, addr, buf, size);
	mutex_unlock(&chip->i2c_lock);
	if (ret < 0)
		mca_log_err("failed-bulk-read, reg(0x%02X), size(%d), ret(%d)\n",
			    addr, size, ret);

	return ret;
}

static int rx1671_write_buffer(struct nuvolta_1671_chg *chip, const u8 *buf,
			       u16 addr, int size)
{
	int ret;

	mutex_lock(&chip->i2c_lock);
	if (chip->shutdown_flag) {
		mca_log_err("failed-bulk-write, device is shutdown\n");
		mutex_unlock(&chip->i2c_lock);
		return 0;
	}
	ret = regmap_bulk_write(chip->regmap, addr, buf, size);
	mutex_unlock(&chip->i2c_lock);
	if (ret < 0)
		mca_log_err("failed-bulk-write, reg(0x%02X), size(%d), ret(%d)\n",
			    addr, size, ret);

	return ret;
}

static int rx1671_read_u16(struct nuvolta_1671_chg *chip, u16 addr, int *val)
{
	u8 buf[2] = { 0 };
	int ret;

	ret = rx1671_read(chip, &buf[0], addr);
	if (ret < 0)
		return ret;
	ret = rx1671_read(chip, &buf[1], addr + 1);
	if (ret < 0)
		return ret;

	*val = buf[0] | (buf[1] << 8);
	return ret;
}

static int nuvolta_1671_vote_enable(struct nuvolta_1671_chg *chip,
				    unsigned int type, bool enable)
{
	int ret = 0;

	mutex_lock(&chip->wireless_enable_lock);

	if (enable)
		chip->disable_votes &= ~type;
	else
		chip->disable_votes |= type;

	mca_log_info("en_type: %d, vote: %d, disable_votes: %d, enable_status: %d\n",
		     type, enable, chip->disable_votes, chip->enable_status);

	if (!!chip->disable_votes != chip->enable_status)
		goto out;

	chip->enable_status = !chip->disable_votes;
	mca_log_info("set enable mode:%d\n", chip->enable_status);

	if (!chip->enable_status)
		rx1671_write(chip, NU1671_SLEEP_CTRL_MAGIC,
			     NU1671_REG_SLEEP_CTRL);

	ret = gpio_direction_output(chip->enable_gpio, !chip->enable_status);
	if (ret)
		mca_log_err("set direction for enable gpio [%d] fail\n",
			    chip->enable_gpio);

	mca_log_info("nuvolta enable gpio val is :%d\n",
		     gpio_get_value(chip->enable_gpio));
out:
	mutex_unlock(&chip->wireless_enable_lock);
	return ret;
}

static int nuvolta_1671_set_enable_mode(bool enable, void *data)
{
	struct nuvolta_1671_chg *chip = data;

	return nuvolta_1671_vote_enable(chip, NU1671_EN_VOTE_MCA, enable);
}

static int nuvolta_1671_check_i2c(struct nuvolta_1671_chg *chip)
{
	u8 data = 0;
	int ret;

	mutex_lock(&chip->data_transfer_lock);
	ret = rx1671_write(chip, NU1671_SLEEP_CTRL_MAGIC, NU1671_REG_SLEEP_CTRL);
	if (ret < 0)
		goto out;

	msleep(20);
	ret = rx1671_read(chip, &data, NU1671_REG_SLEEP_CTRL);
	if (ret < 0)
		goto out;

	if (data == NU1671_SLEEP_CTRL_MAGIC) {
		mca_log_info("i2c check ok!\n");
	} else {
		mca_log_err("i2c check failed!\n");
		ret = -1;
	}
out:
	mutex_unlock(&chip->data_transfer_lock);
	return ret;
}

static int nuvolta_1671_ops_check_i2c(void *data)
{
	return nuvolta_1671_check_i2c(data);
}

static int nuvolta_1671_get_rx_rtx_mode(int *mode, void *data)
{
	struct nuvolta_1671_chg *chip = data;
	u8 val = 0;
	int ret;

	ret = rx1671_read(chip, &val, NU1671_REG_RTX_MODE);
	mca_log_info("data = %d\n", val);
	*mode = (val == NU1671_RTX_MODE_TRX) ? RTX_MODE : RX_MODE;

	return ret;
}

static int nuvolta_1671_enable_reverse_chg(bool enable, void *data)
{
	struct nuvolta_1671_chg *chip = data;
	int ret;

	chip->proc_data.reverse_chg_en = enable;
	ret = rx1671_write(chip, enable, NU1671_REG_TRX_CTRL);
	mca_log_err("enable_reverse_chg: %d\n", enable);

	return ret;
}

static int nuvolta_1671_is_present(int *present, void *data)
{
	struct nuvolta_1671_chg *chip = data;

	*present = chip->proc_data.power_good_flag;
	return 0;
}

static int nuvolta_1671_set_vout(int vout, void *data)
{
	struct nuvolta_1671_chg *chip = data;
	u8 cep = 0;
	int ret = 0;

	if (!chip->proc_data.power_good_flag)
		return -1;

	mutex_lock(&chip->data_transfer_lock);
	msleep(20);

	/* while charging in parallel, don't move vout on a bad link */
	if (chip->proc_data.parallel_charge &&
	    rx1671_read(chip, &cep, NU1671_REG_CEP) >= 0) {
		mca_log_info("cep: %d\n", cep);
		if (cep > NU1671_CEP_MAX) {
			mca_log_info("vol:%d ,cep %d, not set\n", vout, cep);
			goto out;
		}
	}

	if (vout >= NU1671_VOUT_SET_MAX_MV)
		vout = NU1671_VOUT_SET_MAX_MV;
	if (vout < NU1671_VOUT_SET_MIN_MV)
		vout = NU1671_VOUT_SET_DEFAULT_MV;

	ret = rx1671_write(chip, NU1671_CMD_SET_VOUT, NU1671_REG_CMD);
	if (ret < 0)
		goto out;
	ret = rx1671_write(chip, NU1671_VOUT_PACKET_LEN, NU1671_REG_CMD_LEN);
	if (ret < 0)
		goto out;
	ret = rx1671_write(chip, vout & 0xff, NU1671_REG_CMD_DATA(0));
	if (ret < 0)
		goto out;
	ret = rx1671_write(chip, (vout >> 8) & 0xff, NU1671_REG_CMD_DATA(1));
	if (ret < 0)
		goto out;
	ret = rx1671_write(chip, NU1671_VOUT_TRIGGER, NU1671_REG_TRIGGER);
	if (ret < 0)
		goto out;

	chip->proc_data.vout_setted = vout;
	mca_log_info("wls_set_vout: %d\n", vout);
out:
	mutex_unlock(&chip->data_transfer_lock);
	return ret;
}

static bool nuvolta_1671_link_up(struct nuvolta_1671_chg *chip)
{
	return chip->proc_data.power_good_flag ||
	       chip->proc_data.reverse_chg_en;
}

static int nuvolta_1671_get_vout(int *vout, void *data)
{
	struct nuvolta_1671_chg *chip = data;
	u8 cep = 0;
	int ret;

	if (!nuvolta_1671_link_up(chip)) {
		*vout = 0;
		return 0;
	}

	ret = rx1671_read_u16(chip, NU1671_REG_VOUT, vout);
	if (ret < 0)
		return ret;
	ret = rx1671_read(chip, &cep, NU1671_REG_CEP);
	if (ret < 0)
		return ret;

	mca_log_info("wls_get_vout: %d, cep = %d\n", *vout, cep);
	return ret;
}

static int nuvolta_1671_get_iout(int *iout, void *data)
{
	struct nuvolta_1671_chg *chip = data;
	int ret;

	if (!nuvolta_1671_link_up(chip)) {
		*iout = 0;
		return 0;
	}

	ret = rx1671_read_u16(chip, NU1671_REG_IOUT, iout);
	if (ret >= 0)
		mca_log_info("wls_get_iout: %d\n", *iout);

	return ret;
}

static int nuvolta_1671_get_vrect(int *vrect, void *data)
{
	struct nuvolta_1671_chg *chip = data;
	int ret;

	if (!nuvolta_1671_link_up(chip)) {
		*vrect = 0;
		return 0;
	}

	ret = rx1671_read_u16(chip, NU1671_REG_VRECT, vrect);
	if (ret >= 0)
		mca_log_info("wls_get_vrect: %d\n", *vrect);

	return ret;
}

static int nuvolta_1671_get_temp(int *temp, void *data)
{
	struct nuvolta_1671_chg *chip = data;
	int ret;

	if (!nuvolta_1671_link_up(chip)) {
		*temp = 0;
		return 0;
	}

	ret = rx1671_read_u16(chip, NU1671_REG_TEMP, temp);
	if (ret >= 0)
		mca_log_info("wls_get_rx_temp: %d\n", *temp);

	return ret;
}

static int nuvolta_1671_get_tx_adapter(int *adapter, void *data)
{
	struct nuvolta_1671_chg *chip = data;

	*adapter = chip->proc_data.power_good_flag ?
			   chip->proc_data.adapter_type : 0;
	mca_log_info("wls_get_adapter: %d\n", chip->proc_data.adapter_type);

	return 0;
}

static int nuvolta_1671_get_tx_type(int *type, void *data)
{
	struct nuvolta_1671_chg *chip = data;

	*type = chip->proc_data.power_good_flag ? chip->proc_data.tx_type : 0;
	mca_log_info("wls_get_tx_type: %d\n", chip->proc_data.tx_type);

	return 0;
}

static int nuvolta_1671_get_tx_adapter_by_i2c(int *adapter, void *data)
{
	struct nuvolta_1671_chg *chip = data;
	u8 val = 0;
	int ret;

	if (!chip->proc_data.power_good_flag) {
		*adapter = 0;
		return 0;
	}

	ret = rx1671_read(chip, &val, NU1671_REG_ADAPTER_TYPE);
	if (ret < 0)
		return ret;

	*adapter = val;
	mca_log_info("wls_get_adapter_by_i2c: %d\n", *adapter);
	return ret;
}

static int nuvolta_1671_is_car_adapter(bool *enable, void *data)
{
	struct nuvolta_1671_chg *chip = data;

	*enable = chip->proc_data.is_car_tx;
	return 0;
}

static int nuvolta_1671_set_fw_bin(const char *buf, int count, void *data)
{
	static u16 total_length;
	static u8 serial_number;
	static u8 fw_area;
	struct nuvolta_1671_chg *chip = data;

	mca_log_info("buf:%s, count:%d\n", buf, count);
	if (strncmp("length:", buf, 7) == 0) {
		if (kstrtou16(buf + 7, 10, &total_length))
			return -EINVAL;
		chip->fw_bin_length = total_length;
		serial_number = 0;
		mca_log_info("total_length:%d, serial_number:%d\n",
			     total_length, serial_number);
	} else if (strncmp("area:", buf, 5) == 0) {
		if (kstrtou8(buf + 5, 10, &fw_area))
			return -EINVAL;
		mca_log_info("area:%d\n", fw_area);
	} else {
		if (count < 0 ||
		    (serial_number + 1) * count > (int)sizeof(chip->fw_bin))
			return -EINVAL;
		memcpy(chip->fw_bin + serial_number * count, buf, count);
		serial_number++;
		mca_log_info("serial_number:%d, count:%d\n", serial_number,
			     count);
	}

	return count;
}

static int nuvolta_1671_get_rx_int_flag(int *int_flag, void *data)
{
	struct nuvolta_1671_chg *chip = data;

	*int_flag = chip->proc_data.int_flag;
	return 0;
}

static int nuvolta_1671_get_rx_power_mode(u8 *power_mode, void *data)
{
	struct nuvolta_1671_chg *chip = data;
	u8 val = 0;
	int ret;

	ret = rx1671_read(chip, &val, NU1671_REG_POWER_MODE);
	if (ret < 0)
		return ret;

	*power_mode = val;
	mca_log_info("rx power mode: %d\n", val);
	return 0;
}

static int nuvolta_1671_get_tx_max_power(u8 *max_power, void *data)
{
	struct nuvolta_1671_chg *chip = data;
	u8 val = 0;
	int ret;

	ret = rx1671_read(chip, &val, NU1671_REG_TX_MAX_POWER);
	if (ret < 0)
		return ret;

	*max_power = val;
	mca_log_info("tx max power: %d\n", val);
	return 0;
}

static int nuvolta_1671_get_auth_value(int *value, void *data)
{
	struct nuvolta_1671_chg *chip = data;
	u8 auth = 0, val = 0;
	int i, ret;

	ret = rx1671_read(chip, &auth, NU1671_REG_AUTH_DATA);
	if (ret < 0)
		return ret;
	mca_log_info("auth_data = 0x%x\n", auth);

	if (auth) {
		ret = rx1671_read(chip, &chip->proc_data.epp_tx_id_l,
				  NU1671_REG_TX_ID_L);
		if (ret < 0)
			return ret;
		ret = rx1671_read(chip, &chip->proc_data.epp_tx_id_h,
				  NU1671_REG_TX_ID_H);
		if (ret < 0)
			return ret;
	}

	ret = rx1671_read(chip, &val, NU1671_REG_ADAPTER_TYPE);
	if (ret < 0)
		return ret;
	chip->proc_data.adapter_type = val;

	ret = rx1671_read(chip, &chip->proc_data.tx_type, NU1671_REG_TX_TYPE);
	if (ret < 0)
		return ret;

	if (chip->proc_data.adapter_type == 0)
		chip->proc_data.adapter_type = NU1671_ADAPTER_AUTH_FAILED;

	if (auth >= NU1671_AUTH_UUID_VALID) {
		for (i = 0; i < ARRAY_SIZE(chip->proc_data.uuid); i++) {
			ret = rx1671_read(chip, &chip->proc_data.uuid[i],
					  NU1671_REG_UUID + i);
			if (ret < 0)
				return ret;
		}
	}

	*value = auth;

	mca_log_info("tx_id_l: 0x%x, tx_id_h: 0x%x\n",
		     chip->proc_data.epp_tx_id_l, chip->proc_data.epp_tx_id_h);
	mca_log_info("adapter type: %d\n", chip->proc_data.adapter_type);
	mca_log_info("uuid: 0x%x, 0x%x, 0x%x, 0x%x\n", chip->proc_data.uuid[0],
		     chip->proc_data.uuid[1], chip->proc_data.uuid[2],
		     chip->proc_data.uuid[3]);

	return 0;
}

static int nuvolta_1671_set_adapter_voltage(int voltage, void *data)
{
	struct nuvolta_1671_chg *chip = data;
	int ret;

	mutex_lock(&chip->data_transfer_lock);

	if (voltage < NU1671_ADAPTER_VOL_MIN_MV ||
	    voltage > NU1671_ADAPTER_VOL_MAX_MV)
		voltage = NU1671_ADAPTER_VOL_DEFAULT_MV;

	ret = rx1671_write(chip, NU1671_CMD_TRANSMIT_PACKET, NU1671_REG_CMD);
	if (ret < 0)
		goto out;
	ret = rx1671_write(chip, NU1671_ADAPTER_VOL_PACKET_LEN,
			   NU1671_REG_CMD_LEN);
	if (ret < 0)
		goto out;
	ret = rx1671_write(chip, NU1671_ADAPTER_VOL_SET_TYPE,
			   NU1671_REG_CMD_DATA(0));
	if (ret < 0)
		goto out;
	ret = rx1671_write(chip, NU1671_TRANS_DATA_3BYTE,
			   NU1671_REG_CMD_DATA(1));
	if (ret < 0)
		goto out;
	ret = rx1671_write(chip, NU1671_FASTCHG_SET_TX_VOLTAGE,
			   NU1671_REG_CMD_DATA(2));
	if (ret < 0)
		goto out;
	ret = rx1671_write(chip, voltage & 0xff, NU1671_REG_CMD_DATA(3));
	if (ret < 0)
		goto out;
	ret = rx1671_write(chip, (voltage >> 8) & 0xff, NU1671_REG_CMD_DATA(4));
	if (ret < 0)
		goto out;
	ret = rx1671_write(chip, NU1671_ADAPTER_VOL_TRIGGER,
			   NU1671_REG_TRIGGER);
	if (ret < 0)
		goto out;

	mca_log_info("adapter voltage setted: %d\n", voltage);
out:
	mutex_unlock(&chip->data_transfer_lock);
	return ret;
}

static int nuvolta_1671_get_tx_uuid(u8 *uuid, void *data)
{
	struct nuvolta_1671_chg *chip = data;

	if (!chip->proc_data.power_good_flag)
		return -1;

	memcpy(uuid, chip->proc_data.uuid, sizeof(chip->proc_data.uuid));
	return 0;
}

static int nuvolta_1671_get_fod_set(struct nuvolta_1671_chg *chip)
{
	if (!chip->support_hall)
		return FOD_PARAMS_SET_NORMAL;

	if (chip->hall_n_status)
		return chip->hall_s_status ? FOD_PARAMS_SET_NORMAL :
					     FOD_PARAMS_SET_MAG;

	return FOD_PARAMS_SET_MAG_THICK;
}

static int nuvolta_1671_write_fod_to_ic(struct nuvolta_1671_chg *chip,
					struct fod_params_t *fod, int set)
{
	struct params_t *params = fod->params[set];
	u8 len = min_t(u8, fod->length, DEFAULT_FOD_PARAM_LEN);
	u8 buf[NU1671_FOD_CHUNK * PARAMS_T_MAX];
	u8 i, chunk, bytes;
	int ret;

	mutex_lock(&chip->data_transfer_lock);

	for (i = 0; i < len; i++)
		mca_log_info("params[%d]: %d, %d\n", i, params[i].gain,
			     params[i].offset);

	ret = rx1671_write(chip, NU1671_CMD_FOD_SET, NU1671_REG_CMD);
	if (ret < 0)
		goto out;

	mca_log_info("set fod type: %d\n", fod->type);
	ret = rx1671_write(chip, fod->type, NU1671_REG_CMD_DATA(0));
	if (ret < 0)
		goto out;

	for (i = 0; i < len; i += chunk) {
		chunk = (i + NU1671_FOD_CHUNK > len) ? len - i :
						       NU1671_FOD_CHUNK;
		bytes = chunk * PARAMS_T_MAX;

		ret = rx1671_write(chip, bytes + NU1671_FOD_HDR_LEN,
				   NU1671_REG_CMD_LEN);
		if (ret < 0)
			goto out;
		ret = rx1671_write(chip, i, NU1671_REG_CMD_DATA(1));
		if (ret < 0)
			goto out;
		ret = rx1671_write(chip, bytes, NU1671_REG_CMD_DATA(2));
		if (ret < 0)
			goto out;

		memcpy(buf, &params[i], bytes);
		ret = rx1671_write_buffer(chip, buf, NU1671_FOD_DATA_REG,
					  bytes);
		if (ret < 0)
			goto out;
		ret = rx1671_write(chip, bytes + NU1671_FOD_HDR_LEN + 2,
				   NU1671_REG_TRIGGER);
		if (ret < 0)
			goto out;

		msleep(NU1671_FOD_SET_DELAY_MS);
	}
out:
	mutex_unlock(&chip->data_transfer_lock);
	return ret;
}

static int nuvolta_1671_write_bpp_plus_fod_to_ic(struct nuvolta_1671_chg *chip,
						 struct fod_params_t *fod,
						 int set)
{
	struct params_t *params = fod->params[set];
	int i, ret;

	for (i = 0; i < fod->length && i < DEFAULT_FOD_PARAM_LEN; i++)
		mca_log_info("params[%d]: %d, %d\n", i, params[i].gain,
			     params[i].offset);

	mutex_lock(&chip->data_transfer_lock);

	ret = rx1671_write(chip, NU1671_CMD_FOD_SET, NU1671_REG_CMD);
	if (ret < 0)
		goto out;
	ret = rx1671_write(chip, NU1671_BPP_PLUS_FOD_CMD_LEN,
			   NU1671_REG_CMD_LEN);
	if (ret < 0)
		goto out;
	ret = rx1671_write(chip, fod->type, NU1671_REG_CMD_DATA(0));
	if (ret < 0)
		goto out;
	ret = rx1671_write(chip, 0, NU1671_REG_CMD_DATA(1));
	if (ret < 0)
		goto out;
	ret = rx1671_write(chip, NU1671_BPP_PLUS_FOD_BYTES,
			   NU1671_REG_CMD_DATA(2));
	if (ret < 0)
		goto out;
	ret = rx1671_write_buffer(chip, (const u8 *)params, NU1671_FOD_DATA_REG,
				  NU1671_BPP_PLUS_FOD_BYTES);
	if (ret < 0)
		goto out;
	ret = rx1671_write(chip, NU1671_BPP_PLUS_FOD_TRIGGER,
			   NU1671_REG_TRIGGER);
	if (ret < 0)
		goto out;

	mca_log_info("set bpp plus fod type: %d\n", fod->type);
	msleep(NU1671_FOD_SET_DELAY_MS);
out:
	mutex_unlock(&chip->data_transfer_lock);
	return ret;
}

static int nuvolta_1671_get_uuid(struct nuvolta_1671_chg *chip)
{
	return chip->proc_data.uuid[0] << 24 | chip->proc_data.uuid[1] << 16 |
	       chip->proc_data.uuid[2] << 8 | chip->proc_data.uuid[3];
}

static struct fod_params_t *
nuvolta_1671_find_fod(struct fod_params_t *table, int size, int uuid)
{
	int i;

	for (i = 0; i < size && i < FOD_PARA_MAX_GROUP; i++) {
		if (table[i].uuid == uuid)
			return &table[i];
	}

	return NULL;
}

static bool nuvolta_1671_use_bpp_plus(struct nuvolta_1671_chg *chip)
{
	return chip->proc_data.adapter_type == NU1671_ADAPTER_QC3 ||
	       chip->proc_data.adapter_type == NU1671_ADAPTER_PD;
}

static bool nuvolta_1671_use_epp_default(struct nuvolta_1671_chg *chip)
{
	return chip->proc_data.epp &&
	       chip->proc_data.adapter_type >= NU1671_ADAPTER_XIAOMI_QC3;
}

static int nuvolta_1671_set_fod_params(int value, void *data)
{
	struct nuvolta_1671_chg *chip = data;
	int set = nuvolta_1671_get_fod_set(chip);
	int uuid = nuvolta_1671_get_uuid(chip);
	struct fod_params_t *fod = NULL;

	if (chip->proc_data.cp_mode == NU1671_CP_MODE_2_1) {
		fod = nuvolta_1671_find_fod(chip->fod_params_2_1,
					    chip->fod_params_2_1_size, uuid);
		if (fod)
			mca_log_info("set 2_1 fod by uuid: 0x%x,0x%x,0x%x,0x%x\n",
				     chip->proc_data.uuid[0],
				     chip->proc_data.uuid[1],
				     chip->proc_data.uuid[2],
				     chip->proc_data.uuid[3]);
	}

	if (!fod) {
		fod = nuvolta_1671_find_fod(chip->fod_params,
					    chip->fod_params_size, uuid);
		if (fod)
			mca_log_info("set fod by uuid: 0x%x,0x%x,0x%x,0x%x\n",
				     chip->proc_data.uuid[0],
				     chip->proc_data.uuid[1],
				     chip->proc_data.uuid[2],
				     chip->proc_data.uuid[3]);
	}

	if (fod) {
		nuvolta_1671_write_fod_to_ic(chip, fod, set);
		return 0;
	}

	if (nuvolta_1671_use_epp_default(chip)) {
		mca_log_info("set fod epp+ default\n");
		nuvolta_1671_write_fod_to_ic(chip, &chip->fod_params_default,
					     set);
		return 0;
	}

	if (nuvolta_1671_use_bpp_plus(chip)) {
		mca_log_info("set fod bpp plus\n");
		nuvolta_1671_write_bpp_plus_fod_to_ic(
			chip, &chip->fod_params_bpp_plus, set);
		return 0;
	}

	mca_log_info("can not find fod params, uuid: 0x%x,0x%x,0x%x,0x%x\n",
		     chip->proc_data.uuid[0], chip->proc_data.uuid[1],
		     chip->proc_data.uuid[2], chip->proc_data.uuid[3]);
	return -1;
}

static int nuvolta_1671_set_cp_mode(int mode, void *data)
{
	struct nuvolta_1671_chg *chip = data;

	mca_log_err("set cp_mode %d\n", mode);
	chip->proc_data.cp_mode = mode;

	return 0;
}

static int nuvolta_1671_get_rx_fastcharge_status(u8 *fc_flag, void *data)
{
	struct nuvolta_1671_chg *chip = data;
	u8 val = 0;
	int ret;

	ret = rx1671_read(chip, &val, NU1671_REG_FC_RESULT);
	if (ret < 0)
		return ret;

	*fc_flag = val;
	chip->proc_data.fc_flag = val;
	mca_log_info("fastch result: %d\n", *fc_flag);

	return ret;
}

static int nuvolta_1671_receive_transparent_data(u8 *rcv_value, int buff_len,
						 int *rcv_len, void *data)
{
	struct nuvolta_1671_chg *chip = data;
	u8 hdr = 0;
	int length, i, ret;

	ret = rx1671_read(chip, &hdr, NU1671_REG_TRANS_RCV_HDR);
	if (ret < 0)
		return ret;

	*rcv_len = hdr;
	mca_log_info("receive_transparent_data data_length=%d\n", hdr);

	length = (hdr & 0x0f) + (hdr >> 4);
	if (length > buff_len) {
		mca_log_info("receive_transparent_data buffer overflow\n");
		*rcv_len = 0;
		return -1;
	}

	for (i = 0; i < length; i++) {
		ret = rx1671_read(chip, &rcv_value[i],
				  NU1671_REG_TRANS_RCV_DATA + i);
		if (ret < 0)
			return ret;
		mca_log_info("receive_transparent_data i=%d, data[i]=0x%x\n", i,
			     rcv_value[i]);
	}

	return ret;
}

static int nuvolta_1671_send_transparent_data(u8 *send_data, u8 length,
					      void *data)
{
	struct nuvolta_1671_chg *chip = data;
	int i, ret;

	mutex_lock(&chip->data_transfer_lock);

	mca_log_info("data[0] = 0x%x data[1] = 0x%x data[2] = 0x%x, length=%d\n",
		     send_data[0], send_data[1], send_data[2], length);
	msleep(20);

	ret = rx1671_write(chip, NU1671_CMD_TRANSMIT_PACKET, NU1671_REG_CMD);
	if (ret < 0) {
		mca_log_err("FAILED!\n");
		goto out;
	}

	ret = rx1671_write(chip, length, NU1671_REG_CMD_LEN);
	if (ret < 0) {
		mca_log_err("write lenth FAILED!\n");
		goto out;
	}

	for (i = 0; i < length; i++) {
		ret = rx1671_write(chip, send_data[i], NU1671_REG_CMD_DATA(i));
		if (ret < 0) {
			mca_log_err("write data FAILED!\n");
			goto out;
		}
		mca_log_info("i=%d, send_data[0+i]=0x%x, (0x0002+i)=0x%x\n", i,
			     send_data[i], NU1671_REG_CMD_DATA(i));
	}

	ret = rx1671_write(chip, length + 2, NU1671_REG_TRIGGER);
	if (ret < 0)
		mca_log_err("write trigger FAILED!\n");
out:
	mutex_unlock(&chip->data_transfer_lock);
	return ret;
}

static int nuvolta_1671_get_ss_voltage(int *ss_voltage, void *data)
{
	struct nuvolta_1671_chg *chip = data;
	u8 val = 0;
	int ret;

	if (!chip->proc_data.power_good_flag) {
		*ss_voltage = 0;
		return 0;
	}

	ret = rx1671_read(chip, &val, NU1671_REG_SS_VOLTAGE);
	if (ret < 0)
		return ret;

	*ss_voltage = val;
	chip->proc_data.ss_voltage = val;
	mca_log_info("ss_voltage = %d\n", *ss_voltage);

	return ret;
}

static int nuvolta_1671_do_renego(u8 max_power, void *data)
{
	struct nuvolta_1671_chg *chip = data;
	int ret;

	mutex_lock(&chip->data_transfer_lock);
	mca_log_info("max_power = %d\n", max_power);

	ret = rx1671_write(chip, NU1671_CMD_RENEGO, NU1671_REG_CMD);
	if (ret < 0)
		goto out;
	ret = rx1671_write(chip, NU1671_RENEGO_LEN, NU1671_REG_CMD_LEN);
	if (ret < 0)
		goto out;
	ret = rx1671_write(chip, max_power, NU1671_REG_CMD_DATA(0));
	if (ret < 0)
		goto out;
	ret = rx1671_write(chip, NU1671_RENEGO_TRIGGER, NU1671_REG_TRIGGER);
out:
	mutex_unlock(&chip->data_transfer_lock);
	return ret;
}

static int nuvolta_1671_set_parallel_charge(bool parallel, void *data)
{
	struct nuvolta_1671_chg *chip = data;

	chip->proc_data.parallel_charge = parallel;
	mca_log_info("set_parallel_charge: %d\n", parallel);

	return 0;
}

static int nuvolta_1671_get_vout_setted(int *vout_setted, void *data)
{
	struct nuvolta_1671_chg *chip = data;

	*vout_setted = chip->proc_data.vout_setted;
	mca_log_info("vout_setted is %d\n", *vout_setted);

	return 0;
}

static int nuvolta_1671_get_poweroff_err_code(u8 *err_code, void *data)
{
	struct nuvolta_1671_chg *chip = data;
	u8 regs[NU1671_POWEROFF_DUMP_LEN] = { 0 };
	char dump[512];
	int half, i, len, ret;

	ret = rx1671_read_buffer(chip, regs, NU1671_REG_POWEROFF_DUMP,
				 sizeof(regs));
	if (ret < 0) {
		mca_log_err("fail to read poweroff related registers\n");
		return ret;
	}

	for (half = 0; half < 2; half++) {
		len = 0;
		for (i = half * 8; i < half * 8 + 8; i++)
			len += scnprintf(dump + len, sizeof(dump) - len,
					 "[0x%02X]=0x%02X,",
					 NU1671_REG_POWEROFF_DUMP + i, regs[i]);
		mca_log_info("dump: %s\n", dump);
	}

	ret = rx1671_read(chip, err_code, NU1671_REG_CHECK_DATA);
	if (ret < 0)
		mca_log_err("fail to read poweroff error code\n");

	return ret;
}

static int nuvolta_1671_get_project_vendor(int *project_vendor, void *data)
{
	struct nuvolta_1671_chg *chip = data;

	*project_vendor = chip->project_vendor;
	mca_log_info("project_vendor is %d\n", *project_vendor);

	return 0;
}

static int nuvolta_1671_enable_reverse_fod(bool enable, void *data)
{
	struct nuvolta_1671_chg *chip = data;
	int ret;

	mutex_lock(&chip->data_transfer_lock);

	ret = rx1671_write(chip, NU1671_CMD_ENABLE_REVERSE_FOD, NU1671_REG_CMD);
	if (ret < 0)
		goto out;

	if (enable) {
		ret = rx1671_write(chip, 1, NU1671_REG_CMD_LEN);
		if (ret < 0)
			goto out;
		ret = rx1671_write(chip, NU1671_REV_FOD_DEFAULT_GAIN,
				   NU1671_REG_CMD_DATA(0));
		if (ret < 0)
			goto out;
		ret = rx1671_write(chip, 0, NU1671_REG_CMD_DATA(1));
		if (ret < 0)
			goto out;
		ret = rx1671_write(chip, NU1671_REV_FOD_EN_TRIGGER,
				   NU1671_REG_TRIGGER);
		if (ret < 0)
			goto out;
		mca_log_info("gain: %d, offset:%d\n",
			     NU1671_REV_FOD_DEFAULT_GAIN, 0);
	} else {
		ret = rx1671_write(chip, 0, NU1671_REG_CMD_LEN);
		if (ret < 0)
			goto out;
		ret = rx1671_write(chip, NU1671_REV_FOD_DIS_TRIGGER,
				   NU1671_REG_TRIGGER);
		if (ret < 0)
			goto out;
		mca_log_info("disable reverse fod\n");
	}
out:
	mutex_unlock(&chip->data_transfer_lock);
	return ret;
}

static int nuvolta_1671_send_tx_q_value(u8 value, void *data)
{
	struct nuvolta_1671_chg *chip = data;
	u8 buf[4] = { 0x01, 0x28, 0x62, value };
	int ret;

	if (!chip->proc_data.power_good_flag)
		return -1;

	ret = nuvolta_1671_send_transparent_data(buf, sizeof(buf), chip);
	mca_log_info("{0x%02x, 0x%02x, 0x%02x, 0x%02x}\n", buf[0], buf[1],
		     buf[2], buf[3]);

	return ret;
}

static int nuvolta_1671_send_tx_fan_speed(int value, void *data)
{
	struct nuvolta_1671_chg *chip = data;
	u8 buf[5] = { 0x00, 0x38, 0x63, 0x00, 0x00 };
	int ret;

	if (value > NU1671_TX_FAN_SPEED_MAX ||
	    !chip->proc_data.power_good_flag)
		return -1;

	if (chip->proc_data.tx_speed == value)
		return 0;

	buf[3] = value;
	chip->proc_data.tx_speed = value;
	ret = nuvolta_1671_send_transparent_data(buf, sizeof(buf), chip);
	mca_log_info("send_tx_fan_speed_request = %d", chip->proc_data.tx_speed);

	return ret;
}

static int nuvolta_1671_get_tx_fan_speed(int *value, void *data)
{
	struct nuvolta_1671_chg *chip = data;

	*value = chip->proc_data.tx_speed;
	mca_log_info("tx_fan_speed is %d\n", *value);

	return 0;
}

static int nuvolta_1671_set_rx_offset(int rx_offset, void *data)
{
	struct nuvolta_1671_chg *chip = data;

	chip->fake_rx_offset = rx_offset;
	return 0;
}

static int nuvolta_1671_get_rx_offset(int *rx_offset, void *data)
{
	struct nuvolta_1671_chg *chip = data;
	int ss = chip->proc_data.ss_voltage;

	*rx_offset = 0;
	if (!chip->proc_data.power_good_flag)
		return 0;

	if (chip->fake_rx_offset) {
		*rx_offset = 1;
		mca_log_info("rx_offset = %d\n", *rx_offset);
	}

	if (ss > 0 && ss < NU1671_RX_OFFSET_THRESHOLD)
		*rx_offset = 1;

	mca_log_info("rx_offset = %d, ss_voltage = %d", *rx_offset, ss);
	return 0;
}

static int nuvolta_1671_set_rx_sleep_mode(int sleep_for_dam, void *data)
{
	struct nuvolta_1671_chg *chip = data;

	if (!sleep_for_dam)
		return 0;

	return rx1671_write(chip, NU1671_TRX_CTRL_SLEEP, NU1671_REG_TRX_CTRL);
}

static int nuvolta_1671_get_trx_isense(int *isense, void *data)
{
	struct nuvolta_1671_chg *chip = data;
	int ret;

	ret = rx1671_read_u16(chip, NU1671_REG_IOUT, isense);
	if (ret >= 0)
		mca_log_info("trx isense: %d\n", *isense);

	return ret;
}

static int nuvolta_1671_get_trx_vrect(int *vrect, void *data)
{
	struct nuvolta_1671_chg *chip = data;
	int ret;

	ret = rx1671_read_u16(chip, NU1671_REG_VRECT, vrect);
	if (ret >= 0)
		mca_log_info("trx vrect: %d\n", *vrect);

	return ret;
}

static int nuvolta_1671_rcv_product_test_cmd(u8 *rev_data, int *length,
					     void *data)
{
	struct nuvolta_1671_chg *chip = data;
	u8 buf[NU1671_FACTORY_RCV_LEN] = { 0 };
	int len, i, ret;

	ret = rx1671_write(chip, NU1671_READ_REQ_START, NU1671_REG_READ_REQ);
	if (ret < 0)
		return ret;

	ret = rx1671_read_buffer(chip, buf, NU1671_REG_FACTORY_RCV,
				 sizeof(buf));
	if (ret < 0)
		return ret;

	len = min_t(int, buf[0], NU1671_FACTORY_RCV_MAX);
	*length = len;
	mca_log_info("data_length=%d\n", len);

	for (i = 0; i < len; i++) {
		rev_data[i] = buf[i + 1];
		mca_log_info("i=%d, data[i]=0x%x\n", i, rev_data[i]);
	}

	return ret;
}

static int nuvolta_1671_process_factory_cmd(u8 cmd, void *data)
{
	struct nuvolta_1671_chg *chip = data;
	u8 buf[8] = { 0 };
	int val = 0, len;

	switch (cmd) {
	case FACTORY_TEST_CMD_ADAPTER_TYPE:
		buf[0] = 0x02;
		buf[1] = 0x28;
		buf[2] = cmd;
		buf[3] = chip->proc_data.adapter_type;
		len = 4;
		mca_log_err("nuvolta_1671_factory_test --usb_type--%d\n",
			    chip->proc_data.adapter_type);
		break;
	case FACTORY_TEST_CMD_RX_IOUT:
		nuvolta_1671_get_iout(&val, chip);
		buf[0] = 0x02;
		buf[1] = 0x38;
		buf[2] = cmd;
		buf[3] = val & 0xff;
		buf[4] = (val >> 8) & 0xff;
		len = 5;
		mca_log_err("nuvolta_1671_factory_test --rx_iout--0x%x,0x%x iout=%d\n",
			    buf[3], buf[4], val);
		break;
	case FACTORY_TEST_CMD_RX_VOUT:
		nuvolta_1671_get_vout(&val, chip);
		buf[0] = 0x02;
		buf[1] = 0x38;
		buf[2] = cmd;
		buf[3] = val & 0xff;
		buf[4] = (val >> 8) & 0xff;
		len = 5;
		mca_log_err("nuvolta_1671_factory_test --rx_vout--0x%x,0x%x vout=%d\n",
			    buf[3], buf[4], val);
		break;
	case FACTORY_TEST_CMD_RX_CHIP_ID:
		buf[0] = 0x02;
		buf[1] = 0x38;
		buf[2] = cmd;
		buf[3] = NU1671_CHIP_ID_H;
		buf[4] = NU1671_CHIP_ID_L;
		len = 5;
		mca_log_err("nuvolta_1671_factory_test --rx_chip_id--0x%x0x%x\n",
			    chip->wls_fw_data->fw_tx_id,
			    chip->wls_fw_data->fw_boot_id);
		break;
	case FACTORY_TEST_CMD_RX_FW_ID:
		buf[0] = 0x02;
		buf[1] = 0x58;
		buf[2] = cmd;
		buf[3] = 0x00;
		buf[4] = chip->wls_fw_data->fw_rx_id;
		buf[5] = chip->wls_fw_data->fw_rx_id;
		buf[6] = chip->wls_fw_data->fw_rx_id;
		len = 7;
		mca_log_err("nuvolta_1671_factory_test --fw_version--0x%x 0x%x\n",
			    buf[4], buf[5]);
		break;
	case FACTORY_TEST_CMD_REVERSE_REQ:
		buf[0] = 0x02;
		buf[1] = 0x18;
		buf[2] = cmd;
		len = 3;
		mca_log_err("[ factory reverse test] receive request\n");
		break;
	default:
		return -1;
	}

	return nuvolta_1671_send_transparent_data(buf, len, chip);
}

static int nuvolta_1671_get_hall_n_gpio_status(int *status, void *data)
{
	struct nuvolta_1671_chg *chip = data;

	*status = chip->hall_n_status;
	return 0;
}

static int nuvolta_1671_get_hall_s_gpio_status(int *status, void *data)
{
	struct nuvolta_1671_chg *chip = data;

	*status = chip->hall_s_status;
	return 0;
}

static int nuvolta_1671_get_hall_gpio_status(bool *status, void *data)
{
	struct nuvolta_1671_chg *chip = data;

	*status = chip->hall_gpio_status;
	return 0;
}

static int nuvolta_1671_get_magnetic_case_flag(bool *flag, void *data)
{
	struct nuvolta_1671_chg *chip = data;

	*flag = chip->magnetic_case_flag;
	return 0;
}

static int nuvolta_1671_set_debug_fod_params(void *data)
{
	struct nuvolta_1671_chg *chip = data;
	struct fod_params_t *all = chip->wls_debug_all_fod_params;
	int set = nuvolta_1671_get_fod_set(chip);
	int index = chip->wls_debug_one_fod_index;
	struct fod_params_t local, *fod;
	int k;

	if (!chip->proc_data.power_good_flag) {
		mca_log_err("power good flag is false\n");
		return -1;
	}

	if (chip->wls_debug_set_fod_type == WLS_DEBUG_SET_FOD_ALL_DIRECTLY) {
		if (!all)
			return -1;
		memset(&local, 0, sizeof(local));
		local.type = all->type;
		local.length = min_t(u8, all->length, DEFAULT_FOD_PARAM_LEN);
		memcpy(local.params[set], all->params[set],
		       local.length * sizeof(struct params_t));
		nuvolta_1671_write_fod_to_ic(chip, &local, set);
		return 0;
	}

	fod = nuvolta_1671_find_fod(chip->fod_params, chip->fod_params_size,
				    nuvolta_1671_get_uuid(chip));
	if (fod) {
		mca_log_info("set fod by uuid: 0x%x,0x%x,0x%x,0x%x\n",
			     chip->proc_data.uuid[0], chip->proc_data.uuid[1],
			     chip->proc_data.uuid[2], chip->proc_data.uuid[3]);
	} else if (nuvolta_1671_use_epp_default(chip)) {
		mca_log_info("fod epp+ default\n");
		fod = &chip->fod_params_default;
	} else if (nuvolta_1671_use_bpp_plus(chip)) {
		mca_log_info("fod bpp plus\n");
		nuvolta_1671_write_bpp_plus_fod_to_ic(
			chip, &chip->fod_params_bpp_plus, set);
		return 0;
	} else {
		mca_log_info("can not found fod params, uuid: 0x%x,0x%x,0x%x,0x%x\n",
			     chip->proc_data.uuid[0], chip->proc_data.uuid[1],
			     chip->proc_data.uuid[2], chip->proc_data.uuid[3]);
		return 0;
	}

	if (chip->wls_debug_set_fod_type == WLS_DEBUG_SET_FOD_EPP_ONE &&
	    index < fod->length && index < DEFAULT_FOD_PARAM_LEN) {
		fod->params[set][index] = chip->wls_debug_one_fod_param;
	} else if (chip->wls_debug_set_fod_type == WLS_DEBUG_SET_FOD_EPP_ALL &&
		   all && fod->length == all->length) {
		for (k = 0; k < fod->length && k < DEFAULT_FOD_PARAM_LEN; k++)
			fod->params[set][k] = all->params[set][k];
	}

	nuvolta_1671_write_fod_to_ic(chip, fod, set);
	return 0;
}

static struct fod_params_t *
nuvolta_1671_debug_all_params(struct nuvolta_1671_chg *chip)
{
	if (!chip->wls_debug_all_fod_params)
		chip->wls_debug_all_fod_params =
			kzalloc(sizeof(struct fod_params_t), GFP_KERNEL);

	return chip->wls_debug_all_fod_params;
}

static int nuvolta_1671_set_debug_fod(int *args, int count, void *data)
{
	struct nuvolta_1671_chg *chip = data;
	int set = nuvolta_1671_get_fod_set(chip);
	struct fod_params_t *all;
	int index;

	switch (args[0]) {
	case DEBUG_SET_ONE_EPP_FOD:
		mca_log_info("set one epp fod\n");
		chip->wls_debug_set_fod_type = WLS_DEBUG_SET_FOD_EPP_ONE;
		chip->wls_debug_one_fod_index = args[1];
		chip->wls_debug_one_fod_param.gain = args[2];
		chip->wls_debug_one_fod_param.offset = args[3];
		break;
	case DEBUG_SET_ALL_EPP_FOD:
		mca_log_info("set all epp fod\n");
		all = nuvolta_1671_debug_all_params(chip);
		if (!all)
			return -ENOMEM;
		all->length = min(count / 2, DEFAULT_FOD_PARAM_LEN);
		chip->wls_debug_set_fod_type = WLS_DEBUG_SET_FOD_EPP_ALL;
		for (index = 0; index < all->length; index++) {
			all->params[set][index].gain = args[index * 2 + 1];
			all->params[set][index].offset = args[index * 2 + 2];
		}
		break;
	case DEBUG_SET_ALL_FOD:
		mca_log_info("wls debug set all fod\n");
		all = nuvolta_1671_debug_all_params(chip);
		if (!all)
			return -ENOMEM;
		all->type = args[1];
		all->length = clamp(count / 2 - 1, 0, DEFAULT_FOD_PARAM_LEN);
		chip->wls_debug_set_fod_type = WLS_DEBUG_SET_FOD_ALL_DIRECTLY;
		for (index = 0; index < all->length; index++) {
			all->params[set][index].gain = args[index * 2 + 2];
			all->params[set][index].offset = args[index * 2 + 3];
		}
		break;
	default:
		mca_log_err("not support debug_fod_type, return\n");
		return 0;
	}

	nuvolta_1671_set_debug_fod_params(chip);
	return 0;
}

static int nuvolta_1671_get_debug_fod_type(WLS_DEBUG_SET_FOD_TYPE *type,
					   void *data)
{
	struct nuvolta_1671_chg *chip = data;

	*type = chip->wls_debug_set_fod_type;
	return 0;
}

static int nuvolta_1671_set_tx_cust_operation(void *data)
{
	struct nuvolta_1671_chg *chip = data;
	int ret;

	ret = rx1671_write(chip, NU1671_CMD_TX_CUST, NU1671_REG_CMD);
	if (ret < 0)
		mca_log_info("set tx authen failed: %d\n", ret);

	ret = rx1671_write(chip, NU1671_TX_CUST_TRIGGER, NU1671_REG_TRIGGER);
	if (ret < 0)
		mca_log_info("send int to trx failed: %d\n", ret);
	else
		mca_log_info("set tx authen success\n");

	mca_log_info("set tx authen ret: %d\n", ret);
	return 0;
}

static u8 nuvolta_1671_image_fw_version(struct nuvolta_1671_chg *chip)
{
	return ~chip->fw_data_ptr[NU1671_FW_VERSION_OFFSET];
}

static int nuvolta_1671_get_saved_fw_version(char *buf, void *data)
{
	struct nuvolta_1671_chg *chip = data;
	u8 ver = chip->wls_fw_data->fw_rx_id;

	scnprintf(buf, 10, "%02x.%02x.%02x", ver, ver, ver);
	mca_log_info("rx firmware version: 0x%x\n", ver);

	return 0;
}

static int nuvolta_1671_get_saved_fw_version_int(int *version, void *data)
{
	struct nuvolta_1671_chg *chip = data;

	*version = chip->wls_fw_data->fw_rx_id;
	return 0;
}

static int nuvolta_1671_check_ic_fw_version(u8 *check_result, void *data)
{
	struct nuvolta_1671_chg *chip = data;
	u8 img_ver = nuvolta_1671_image_fw_version(chip);
	u8 buf[NU1671_FW_CHECK_LEN] = { 0 };
	int ret;

	ret = rx1671_write(chip, NU1671_TRX_CTRL_FW_CHECK, NU1671_REG_TRX_CTRL);
	if (ret < 0)
		return ret;

	msleep(20);
	ret = rx1671_read_buffer(chip, buf, NU1671_REG_CHECK_DATA,
				 sizeof(buf));
	if (ret < 0)
		return ret;

	mca_log_info("fw crc check: 0x%x\n", buf[0]);

	if (buf[0] == NU1671_FW_CHECK_CRC_OK && buf[4] >= img_ver) {
		chip->wls_fw_data->fw_rx_id = buf[4];
		*check_result = RX_CHECK_SUCCESS | TX_CHECK_SUCCESS |
				BOOT_CHECK_SUCCESS;
	} else {
		chip->wls_fw_data->fw_rx_id = 0xfe;
		*check_result = 0;
	}

	mca_log_info("image fw version: 0x%x\n", img_ver);
	mca_log_info("ic fw version: 0x%x\n", chip->wls_fw_data->fw_rx_id);

	return ret;
}

static int nuvolta_1671_check_ic_fw_version_int(int *version, void *data)
{
	u8 result = 0;
	int ret;

	ret = nuvolta_1671_check_ic_fw_version(&result, data);
	*version = result;

	return ret;
}

static int nuvolta_1671_check_fw_update(bool *update, void *data)
{
	struct nuvolta_1671_chg *chip = data;
	u8 ic_ver = 0xfe, img_ver;
	int ret;

	if (!chip->proc_data.power_good_flag)
		return -1;

	ret = rx1671_write(chip, NU1671_READ_REQ_START, NU1671_REG_READ_REQ);
	if (ret >= 0)
		ret = rx1671_read(chip, &ic_ver, NU1671_REG_IC_FW_VER);
	if (ret < 0) {
		mca_log_err("i2c error!\n");
		*update = false;
		return ret;
	}

	chip->wls_fw_data->fw_rx_id = ic_ver;
	img_ver = nuvolta_1671_image_fw_version(chip);
	mca_log_info("ic fw version: 0x%x, image fw version: 0x%x\n", ic_ver,
		     img_ver);

	if (ic_ver < img_ver) {
		mca_log_info("need update\n");
		*update = true;
	} else {
		mca_log_info("no need update\n");
		*update = false;
	}

	return ret;
}

static int nuvolta_1671_mtp_program(struct nuvolta_1671_chg *chip,
				    const u8 *fw)
{
	static const u8 dtm_key0[] = { 0x00, 0x2d, 0xd2, 0x22, 0xdd };
	static const u8 dtm_key1[] = { 0x00, 0x4b, 0xb4, 0x44, 0xbb };
	u8 *arr = fw_arrange_data;
	u8 val = 0;
	int i, retry, ret = 0;

	for (i = 0; i < ARRAY_SIZE(dtm_key0); i++)
		ret |= rx1671_write(chip, dtm_key0[i], NU1671_REG_DTM_UNLOCK0);
	if (ret)
		goto fail;
	ret = rx1671_read(chip, &val, NU1671_REG_DTM_UNLOCK0);
	if (ret < 0)
		goto fail;

	for (i = 0; i < ARRAY_SIZE(dtm_key1); i++)
		ret |= rx1671_write(chip, dtm_key1[i], NU1671_REG_DTM_UNLOCK1);
	if (ret)
		goto fail;
	ret = rx1671_read(chip, &val, NU1671_REG_DTM_UNLOCK1);
	if (ret < 0)
		goto fail;

	ret = rx1671_write(chip, 0xc8, NU1671_REG_MCU_CTRL);
	if (ret < 0)
		goto fail;
	ret = rx1671_read(chip, &val, NU1671_REG_MTP_CFG);
	if (ret < 0)
		goto fail;

	ret = rx1671_write(chip, 0x82, NU1671_REG_MTP_CFG);
	ret |= rx1671_write(chip, 0x00, NU1671_REG_DTM_UNLOCK1);
	if (ret < 0)
		goto fail;
	ret = rx1671_read(chip, &val, NU1671_REG_MTP_CFG);
	if (ret < 0)
		goto fail;

	ret = rx1671_write(chip, 0x40, NU1671_REG_MTP_SLEEP);
	ret |= rx1671_write(chip, 0x01, NU1671_REG_ANA_4A28);
	ret |= rx1671_write(chip, 0x08, NU1671_REG_ANA_4A43);
	ret |= rx1671_write(chip, 0x44, NU1671_REG_ANA_4A32);
	ret |= rx1671_write(chip, 0x1b, NU1671_REG_ANA_4A33);
	ret |= rx1671_write(chip, 0x21, NU1671_REG_ANA_4A30);
	ret |= rx1671_write(chip, 0x80, NU1671_REG_ANA_4918);
	if (ret)
		goto fail;
	ret = rx1671_read(chip, &val, NU1671_REG_MTP_CFG);
	if (ret < 0)
		goto fail;

	ret = rx1671_write(chip, 0x10, NU1671_REG_MTP_CTRL_MODE);
	ret |= rx1671_write(chip, 0x00, NU1671_REG_MTP_CTRL1);
	ret |= rx1671_write(chip, 0x0f, NU1671_REG_MTP_SECTOR);
	if (ret)
		goto fail;

	for (i = 0; i < NU1671_FW_SIZE; i += 4) {
		if (fw) {
			arr[i] = fw[i + 3];
			arr[i + 1] = fw[i + 2];
			arr[i + 2] = fw[i + 1];
			arr[i + 3] = fw[i];
		} else {
			memset(&arr[i], 0xff, 4);
		}
	}

	ret = rx1671_write(chip, 0x00, NU1671_REG_MTP_START_ADDR_L);
	ret |= rx1671_write(chip, 0x00, NU1671_REG_MTP_START_ADDR_H);
	if (ret)
		goto fail;

	for (i = 0; i < 4; i++)
		mca_log_info("fw_arrange_data[%d]:0x%x\n", i, arr[i]);

	for (i = 0; i < 4; i++)
		ret |= rx1671_write(chip, arr[i], NU1671_REG_MTP_DATA(i));
	if (ret)
		goto fail;

	ret = rx1671_write(chip, 0x01, NU1671_REG_MTP_CTRL0);
	ret |= rx1671_write(chip, 0x01, NU1671_REG_MTP_CTRL1);
	ret |= rx1671_write(chip, 0x00, NU1671_REG_MTP_CTRL1);
	if (ret)
		goto fail;

	for (retry = 0; retry < NU1671_MTP_STATUS_RETRY; retry++) {
		ret = rx1671_read(chip, &val, NU1671_REG_MTP_STATUS);
		if (ret < 0)
			goto fail;
		if (!(val & NU1671_MTP_STATUS_BUSY)) {
			if (val & NU1671_MTP_STATUS_ERR) {
				mca_log_err("wrfail, MTP error\n");
				ret = -1;
				goto fail;
			}
			break;
		}
		usleep_range(100, 110);
	}

	ret = rx1671_write(chip, 0x6a, NU1671_REG_MTP_CTRL2);
	if (ret)
		goto fail;

	mca_log_err("start write FW data\n");
	for (i = 0; i < NU1671_FW_SIZE; i += 2 * NU1671_MTP_BURST) {
		ret = rx1671_write_buffer(chip, &arr[i], NU1671_REG_MTP_BUF0,
					  NU1671_MTP_BURST);
		if (ret < 0)
			goto fail;
		ret = rx1671_write(chip, 0x02, NU1671_REG_MTP_CTRL1);
		ret |= rx1671_write(chip, 0x06, NU1671_REG_MTP_CTRL1);
		if (ret)
			goto fail;
		for (retry = 0; retry < NU1671_MTP_BUSY_RETRY; retry++) {
			ret = rx1671_read(chip, &val, NU1671_REG_MTP_BUSY);
			if (ret < 0)
				goto fail;
			if (!val)
				break;
			msleep(3);
		}

		ret = rx1671_write_buffer(chip, &arr[i + NU1671_MTP_BURST],
					  NU1671_REG_MTP_BUF1,
					  NU1671_MTP_BURST);
		ret |= rx1671_write(chip, 0x02, NU1671_REG_MTP_CTRL1);
		ret |= rx1671_write(chip, 0x0a, NU1671_REG_MTP_CTRL1);
		if (ret)
			goto fail;
		for (retry = 0; retry < NU1671_MTP_BUSY_RETRY; retry++) {
			ret = rx1671_read(chip, &val, NU1671_REG_MTP_BUSY);
			if (ret < 0)
				goto fail;
			if (!val)
				break;
			msleep(3);
		}
	}

	ret = rx1671_read(chip, &val, NU1671_REG_MTP_BUSY);
	if (ret < 0)
		goto fail;
	mca_log_err("reg0x00a4: 0x%x\n", val);

	ret = rx1671_write(chip, 0x00, NU1671_REG_MTP_CTRL0);
	ret |= rx1671_write(chip, 0x00, NU1671_REG_MTP_CTRL2);
	ret |= rx1671_write(chip, 0x00, NU1671_REG_ANA_4918);
	ret |= rx1671_write(chip, 0x00, NU1671_REG_ANA_4A30);
	ret |= rx1671_write(chip, 0x08, NU1671_REG_MCU_CTRL);
	ret |= rx1671_write(chip, 0x01, NU1671_REG_RST_5000);
	ret |= rx1671_write(chip, 0x03, NU1671_REG_RST_5008);
	ret |= rx1671_write(chip, 0x00, NU1671_REG_MTP_SLEEP);
	ret |= rx1671_write(chip, 0x00, NU1671_REG_MCU_CTRL);
	ret |= rx1671_write(chip, 0x00, NU1671_REG_DTM_UNLOCK0);
	if (ret)
		goto fail;

	mca_log_err("MTP download finished!\n");
	msleep(100);
	return 0;

fail:
	mca_log_err("wrfail, MTP error\n");
	msleep(100);
	return ret;
}

static int nuvolta_1671_erase_fw_data(void *data)
{
	struct nuvolta_1671_chg *chip = data;
	int ret;

	mca_log_err("start erasing firmware\n");
	ret = nuvolta_1671_mtp_program(chip, NULL);
	msleep(200);

	return ret;
}

static int nuvolta_1671_download_fw_from_image(void *data)
{
	struct nuvolta_1671_chg *chip = data;
	int ret;

	mca_log_err("start downloading firmware from image\n");
	ret = nuvolta_1671_mtp_program(chip, NULL);
	if (ret) {
		mca_log_err("fail to erase before downloading firmware\n");
		return ret;
	}

	msleep(100);
	ret = nuvolta_1671_mtp_program(chip, chip->fw_data_ptr);
	msleep(200);

	return ret;
}

static int nuvolta_1671_download_fw_from_bin(void *data)
{
	struct nuvolta_1671_chg *chip = data;
	int i, ret;

	for (i = 0; i < chip->fw_bin_length && i < (int)sizeof(chip->fw_bin);
	     i++)
		chip->fw_bin[i] = ~chip->fw_bin[i];

	mca_log_err("start downloading firmware from bin\n");
	ret = nuvolta_1671_mtp_program(chip, NULL);
	if (ret) {
		mca_log_err("fail to erase before downloading firmware\n");
		return ret;
	}

	msleep(100);
	ret = nuvolta_1671_mtp_program(chip, chip->fw_bin);
	msleep(200);
	chip->fw_bin_length = 0;

	return ret;
}

static struct platform_class_wireless_ops nuvolta_1671_wls_ops = {
	.wls_enable_reverse_chg = nuvolta_1671_enable_reverse_chg,
	.wls_is_present = nuvolta_1671_is_present,
	.wls_set_vout = nuvolta_1671_set_vout,
	.wls_get_vout = nuvolta_1671_get_vout,
	.wls_get_iout = nuvolta_1671_get_iout,
	.wls_get_vrect = nuvolta_1671_get_vrect,
	.wls_get_temp = nuvolta_1671_get_temp,
	.wls_get_tx_adapter = nuvolta_1671_get_tx_adapter,
	.wls_get_tx_type = nuvolta_1671_get_tx_type,
	.wls_get_tx_adapter_by_i2c = nuvolta_1671_get_tx_adapter_by_i2c,
	.wls_set_enable_mode = nuvolta_1671_set_enable_mode,
	.wls_is_car_adapter = nuvolta_1671_is_car_adapter,
	.wls_set_fw_bin = nuvolta_1671_set_fw_bin,
	.wls_get_rx_rtx_mode = nuvolta_1671_get_rx_rtx_mode,
	.wls_get_rx_int_flag = nuvolta_1671_get_rx_int_flag,
	.wls_get_rx_power_mode = nuvolta_1671_get_rx_power_mode,
	.wls_get_tx_max_power = nuvolta_1671_get_tx_max_power,
	.wls_get_auth_value = nuvolta_1671_get_auth_value,
	.wls_set_adapter_voltage = nuvolta_1671_set_adapter_voltage,
	.wls_get_tx_uuid = nuvolta_1671_get_tx_uuid,
	.wls_set_fod_params = nuvolta_1671_set_fod_params,
	.wls_get_rx_fastcharge_status = nuvolta_1671_get_rx_fastcharge_status,
	.wls_receive_transparent_data = nuvolta_1671_receive_transparent_data,
	.wls_send_transparent_data = nuvolta_1671_send_transparent_data,
	.wls_get_ss_voltage = nuvolta_1671_get_ss_voltage,
	.wls_do_renego = nuvolta_1671_do_renego,
	.wls_set_parallel_charge = nuvolta_1671_set_parallel_charge,
	.wls_get_vout_setted = nuvolta_1671_get_vout_setted,
	.wls_get_poweroff_err_code = nuvolta_1671_get_poweroff_err_code,
	.wls_get_project_vendor = nuvolta_1671_get_project_vendor,
	.wls_get_fw_version = nuvolta_1671_get_saved_fw_version,
	.wls_get_saved_fw_version = nuvolta_1671_get_saved_fw_version_int,
	.wls_check_i2c_is_ok = nuvolta_1671_ops_check_i2c,
	.wls_enable_rev_fod = nuvolta_1671_enable_reverse_fod,
	.wls_send_tx_q_value = nuvolta_1671_send_tx_q_value,
	.wls_download_fw_from_bin = nuvolta_1671_download_fw_from_bin,
	.wls_erase_fw = nuvolta_1671_erase_fw_data,
	.wls_get_fw_version_check = nuvolta_1671_check_ic_fw_version,
	.wls_check_ic_fw_version = nuvolta_1671_check_ic_fw_version_int,
	.wls_download_fw = nuvolta_1671_download_fw_from_image,
	.wls_download_fw_from_image = nuvolta_1671_download_fw_from_image,
	.wls_receive_test_cmd = nuvolta_1671_rcv_product_test_cmd,
	.wls_process_factory_cmd = nuvolta_1671_process_factory_cmd,
	.wls_get_hall_n_gpio_status = nuvolta_1671_get_hall_n_gpio_status,
	.wls_get_hall_s_gpio_status = nuvolta_1671_get_hall_s_gpio_status,
	.wls_get_hall_gpio_status = nuvolta_1671_get_hall_gpio_status,
	.wls_get_magnetic_case_flag = nuvolta_1671_get_magnetic_case_flag,
	.wls_check_firmware_state = nuvolta_1671_check_fw_update,
	.wls_check_fw_update = nuvolta_1671_check_fw_update,
	.wls_set_debug_fod = nuvolta_1671_set_debug_fod,
	.wls_get_debug_fod_type = nuvolta_1671_get_debug_fod_type,
	.wls_set_debug_fod_params = nuvolta_1671_set_debug_fod_params,
	.wls_set_tx_cust_operation = nuvolta_1671_set_tx_cust_operation,
	.wls_set_tx_fan_speed = nuvolta_1671_send_tx_fan_speed,
	.wls_get_tx_fan_speed = nuvolta_1671_get_tx_fan_speed,
	.wls_get_rx_offset = nuvolta_1671_get_rx_offset,
	.wls_set_rx_offset = nuvolta_1671_set_rx_offset,
	.wls_set_rx_sleep_mode = nuvolta_1671_set_rx_sleep_mode,
	.wls_get_trx_isense = nuvolta_1671_get_trx_isense,
	.wls_get_trx_vrect = nuvolta_1671_get_trx_vrect,
	.wls_set_cp_mode = nuvolta_1671_set_cp_mode,
};

static const struct int_map_t rtx_irq_map[] = {
	DECL_INTERRUPT_MAP(BIT(0), RTX_INT_PING),
	DECL_INTERRUPT_MAP(BIT(1), RTX_INT_GET_RX),
	DECL_INTERRUPT_MAP(BIT(2), RTX_INT_CEP_TIMEOUT),
	DECL_INTERRUPT_MAP(BIT(3), RTX_INT_EPT),
	DECL_INTERRUPT_MAP(BIT(4), RTX_INT_PROTECTION),
	DECL_INTERRUPT_MAP(BIT(5), RTX_INT_GET_TX),
	DECL_INTERRUPT_MAP(BIT(6), RTX_INT_REVERSE_TEST_READY),
	DECL_INTERRUPT_MAP(BIT(7), RTX_INT_REVERSE_TEST_DONE),
	DECL_INTERRUPT_MAP(BIT(8), RTX_INT_FOD),
	DECL_INTERRUPT_MAP(BIT(10), RTX_INT_TX_CHS_UPDATE),
	DECL_INTERRUPT_MAP(BIT(11), RTX_INT_TX_BLE_CONNECT),
};

static int nuvolta_1671_config_rtx_int_flag(u16 int_val)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(rtx_irq_map); i++) {
		if (int_val & rtx_irq_map[i].irq_regval)
			return rtx_irq_map[i].irq_flag;
	}

	return RTX_INT_UNKNOWN;
}

static int nuvolta_1671_config_rx_int_flag(u16 int_val)
{
	u32 mask = int_val & GENMASK(10, 0);

	if (int_val & BIT(12))
		mask |= BIT(RX_INT_OCP_OTP_ALARM - 1);
	if (int_val & BIT(13))
		mask |= BIT(RX_INT_FIRST_AUTHEN - 1);
	if (int_val & BIT(14))
		mask |= BIT(RX_INT_POWER_OFF - 1) | BIT(RX_INT_POWER_ON - 1);

	return mask << 1;
}

static void nuvolta_1671_clear_int(struct nuvolta_1671_chg *chip, u8 int_l,
				   u8 int_h)
{
	int ret;

	mutex_lock(&chip->data_transfer_lock);
	ret = rx1671_write(chip, NU1671_CMD_CLEAR_INT, NU1671_REG_CMD);
	ret |= rx1671_write(chip, NU1671_CLEAR_INT_LEN, NU1671_REG_CMD_LEN);
	ret |= rx1671_write(chip, 0xff, NU1671_REG_CMD_DATA(0));
	ret |= rx1671_write(chip, 0xff, NU1671_REG_CMD_DATA(1));
	ret |= rx1671_write(chip, NU1671_CLEAR_INT_TRIGGER, NU1671_REG_TRIGGER);
	mca_log_info("clear int ret: %d, int_flag_lsb = %d, int_flag_msb = %d\n",
		     ret, int_l, int_h);
	mutex_unlock(&chip->data_transfer_lock);
}

static void nuvolta_1671_interrupt_work(struct work_struct *work)
{
	struct nuvolta_1671_chg *chip = container_of(
		work, struct nuvolta_1671_chg, interrupt_work.work);
	u8 int_l = 0, int_h = 0;
	u16 int_val;

	mutex_lock(&chip->wireless_chg_int_lock);

	if (rx1671_read(chip, &int_l, NU1671_REG_INT_L) < 0)
		mca_log_err("read int 0x20 error\n");
	if (rx1671_read(chip, &int_h, NU1671_REG_INT_H) < 0)
		mca_log_err("read int 0x21 error\n");

	int_val = (int_h << 8) | int_l;
	mca_log_info("int_flag: 0x%x\n", int_val);

	nuvolta_1671_get_rx_rtx_mode(&chip->proc_data.int_trx_mode, chip);

	if (chip->proc_data.int_trx_mode == RTX_MODE) {
		chip->proc_data.int_flag =
			nuvolta_1671_config_rtx_int_flag(int_val);
		mca_strategy_func_process(STRATEGY_FUNC_TYPE_REV_WIRELESS,
					  MCA_EVENT_WIRELESS_INT_CHANGE,
					  chip->proc_data.int_flag);
	} else {
		chip->proc_data.int_flag =
			nuvolta_1671_config_rx_int_flag(int_val);
		mca_log_info("int_flag: 0x%x\n", chip->proc_data.int_flag);
		mca_strategy_func_process(STRATEGY_FUNC_TYPE_BASIC_WIRELESS,
					  MCA_EVENT_WIRELESS_INT_CHANGE,
					  chip->proc_data.int_flag);
	}
	chip->proc_data.int_flag = 0;

	nuvolta_1671_clear_int(chip, int_l, int_h);

	mutex_unlock(&chip->wireless_chg_int_lock);
	mca_log_info("already clear int and unlock mutex\n");
}

static irqreturn_t nuvolta_1671_interrupt_handler(int irq, void *dev_id)
{
	struct nuvolta_1671_chg *chip = dev_id;

	schedule_delayed_work(&chip->interrupt_work, 0);
	return IRQ_HANDLED;
}

static void nuvolta_1671_pg_det_work(struct work_struct *work)
{
	struct nuvolta_1671_chg *chip = container_of(
		work, struct nuvolta_1671_chg, pg_detect_work.work);

	if (!gpio_is_valid(chip->power_good_gpio))
		return;

	if (gpio_get_value(chip->power_good_gpio)) {
		if (chip->proc_data.power_good_flag) {
			mca_log_info("wireless is already attached\n");
			return;
		}
		mca_log_info("power_good high, wireless attached\n");
		chip->proc_data.power_good_flag = true;
		mca_event_block_notify(MCA_EVENT_TYPE_CHARGER_CONNECT,
				       MCA_EVENT_WIRELESS_CONNECT, NULL);
	} else {
		mca_log_info("power_good low, wireless detached\n");
		chip->proc_data.power_good_flag = false;
		mca_event_block_notify(MCA_EVENT_TYPE_CHARGER_CONNECT,
				       MCA_EVENT_WIRELESS_DISCONNECT, NULL);
	}
}

static irqreturn_t nuvolta_1671_power_good_handler(int irq, void *dev_id)
{
	struct nuvolta_1671_chg *chip = dev_id;

	mca_log_info("power_good detected\n");
	schedule_delayed_work(&chip->pg_detect_work, 0);

	return IRQ_HANDLED;
}

static void nuvolta_1671_init_detect_work(struct work_struct *work)
{
	struct nuvolta_1671_chg *chip = container_of(
		work, struct nuvolta_1671_chg, init_detect_work.work);
	int pg;

	if (!gpio_is_valid(chip->power_good_gpio))
		return;

	pg = gpio_get_value(chip->power_good_gpio);
	mca_log_info("init power good: %d\n", pg);
	if (!pg)
		return;

	nuvolta_1671_vote_enable(chip, NU1671_EN_VOTE_SELF, false);
	usleep_range(120000, 150000);
	nuvolta_1671_vote_enable(chip, NU1671_EN_VOTE_SELF, true);
}

static void nuvolta_1671_hall_interrupt_work(struct work_struct *work)
{
	struct nuvolta_1671_chg *chip = container_of(
		work, struct nuvolta_1671_chg, hall_interrupt_work.work);
	char event[MCA_EVENT_NOTIFY_SIZE] = { 0 };
	struct mca_event_notify_data event_data;
	bool attached;
	int len;

	if (gpio_is_valid(chip->hall_n_int_gpio))
		chip->hall_n_status = !!gpio_get_value(chip->hall_n_int_gpio);
	if (gpio_is_valid(chip->hall_s_int_gpio))
		chip->hall_s_status = !!gpio_get_value(chip->hall_s_int_gpio);
	mca_log_info("hall interrupt, n: %d, s: %d\n", chip->hall_n_status,
		     chip->hall_s_status);

	attached = !(chip->hall_n_status && chip->hall_s_status);
	chip->hall_gpio_status = !attached;
	if (attached == chip->magnetic_case_flag) {
		mca_log_info("magnetic case already %s\n",
			     attached ? "attached" : "detached");
		return;
	}

	chip->magnetic_case_flag = attached;
	mca_log_info("magnetic case %s\n", attached ? "attached" : "detached");
	mca_strategy_func_process(STRATEGY_FUNC_TYPE_BASIC_WIRELESS,
				  MCA_EVENT_WIRELESS_MAGNETIC_CASE_INT,
				  chip->magnetic_case_flag);

	len = snprintf(event, MCA_EVENT_NOTIFY_SIZE, "POWER_SUPPLY_WLS_HALL=%d",
		       chip->magnetic_case_flag);
	event_data.event = event;
	event_data.event_len = len;
	mca_event_report_uevent(&event_data);
}

static irqreturn_t nuvolta_1671_hall_interrupt_handler(int irq, void *dev_id)
{
	struct nuvolta_1671_chg *chip = dev_id;

	mca_log_info("hall_int detected\n");
	schedule_delayed_work(&chip->hall_interrupt_work, 0);

	return IRQ_HANDLED;
}

static int nuvolta_1671_parse_params(struct device_node *node, const char *name,
				     struct params_t *params)
{
	u8 *idata;
	int len, i;

	if (strcmp(name, "null") == 0) {
		mca_log_info("no need parse params\n");
		return -1;
	}

	len = mca_parse_dts_u8_count(node, name, DEFAULT_FOD_PARAM_LEN,
				     PARAMS_T_MAX);
	if (len < 0) {
		mca_log_err("parse %s failed\n", name);
		return -1;
	}

	idata = kcalloc(len, sizeof(u8), GFP_KERNEL);
	if (!idata)
		return -ENOMEM;

	mca_parse_dts_u8_array(node, name, idata, len);
	for (i = 0; i < len / PARAMS_T_MAX; i++) {
		params[i].gain = idata[PARAMS_T_MAX * i + PARAMS_T_GAIN];
		params[i].offset = idata[PARAMS_T_MAX * i + PARAMS_T_OFFSET];
		mca_log_info("[%d]params: gain:%d, offset:%d\n", i,
			     params[i].gain, params[i].offset);
	}

	kfree(idata);
	return 0;
}

static int nuvolta_1671_parse_fod_table(struct device_node *node,
					const char *prop,
					struct fod_params_t *table, int max_rows,
					int *size)
{
	const char *tmp_string = NULL;
	int array_len, row, col, i;

	array_len = mca_parse_dts_count_strings(node, prop, FOD_PARA_MAX_GROUP,
						FOD_PARA_MAX);
	if (array_len < 0) {
		mca_log_err("parse %s failed\n", prop);
		return -1;
	}

	array_len = min(array_len, max_rows * FOD_PARA_MAX);
	if (size)
		*size = array_len / FOD_PARA_MAX;

	for (i = 0; i < array_len; i++) {
		if (mca_parse_dts_string_index(node, prop, i, &tmp_string))
			return -1;

		row = i / FOD_PARA_MAX;
		col = i % FOD_PARA_MAX;

		mca_log_debug("[%d]%s %s\n", i, prop, tmp_string);
		switch (col) {
		case FOD_PARA_TYPE:
			if (kstrtou8(tmp_string, 10, &table[row].type))
				return -1;
			break;
		case FOD_PARA_LENGTH:
			if (kstrtou8(tmp_string, 10, &table[row].length))
				return -1;
			break;
		case FOD_PARA_UUID:
			if (kstrtoint(tmp_string, 16, &table[row].uuid))
				return -1;
			break;
		case FOD_PARA_PARAMS:
		case FOD_PARA_PARAMS_MAG:
		case FOD_PARA_PARAMS_MAG_THICK:
			if (nuvolta_1671_parse_params(
				    node, tmp_string,
				    table[row].params[col - FOD_PARA_PARAMS]))
				return -1;
			break;
		default:
			break;
		}
	}

	return 0;
}

static int nuvolta_1671_parse_fw_fod_data(struct nuvolta_1671_chg *chip)
{
	struct device_node *node =
		of_find_node_by_name(NULL, "mca_nu1671_fod_data");
	int ret = 0;

	if (chip->project_vendor != WLS_CHIP_VENDOR_NU1671)
		mca_log_err("unsupported project vendor(%d), default to nuvolta_1671 firmware\n",
			    chip->project_vendor);

	if (chip->fw_version_index < 0 ||
	    chip->fw_version_index >= (int)ARRAY_SIZE(fw_data_1671)) {
		mca_log_err("fw_version_index %d out of range, use 0\n",
			    chip->fw_version_index);
		chip->fw_version_index = 0;
	}
	chip->fw_data_ptr = fw_data_1671[chip->fw_version_index];
	chip->fw_data_size = sizeof(fw_data_1671[chip->fw_version_index]);
	mca_log_info("cur fw_data is:[0]:%02x; [1]:%02x; [2]:%02x\n",
		     chip->fw_data_ptr[0], chip->fw_data_ptr[1],
		     chip->fw_data_ptr[2]);

	if (!node) {
		mca_log_err("mca_nu1671_fod_data not found\n");
		return -EINVAL;
	}

	if (nuvolta_1671_parse_fod_table(node, "fod_params", chip->fod_params,
					 FOD_PARA_MAX_GROUP,
					 &chip->fod_params_size)) {
		mca_log_err("parse fod_params failed\n");
		ret = -1;
	}
	if (nuvolta_1671_parse_fod_table(node, "fod_params_default",
					 &chip->fod_params_default, 1, NULL)) {
		mca_log_err("parse fod_params_default failed\n");
		ret = -1;
	}
	if (nuvolta_1671_parse_fod_table(node, "fod_params_bpp_plus",
					 &chip->fod_params_bpp_plus, 1, NULL)) {
		mca_log_err("parse fod_params_bpp_plus failed\n");
		ret = -1;
	}
	if (nuvolta_1671_parse_fod_table(node, "fod_params_2_1",
					 chip->fod_params_2_1,
					 FOD_PARA_MAX_GROUP,
					 &chip->fod_params_2_1_size)) {
		mca_log_err("parse fod_params_2_1 failed\n");
		ret = -1;
	}

	of_node_put(node);
	return ret;
}

static const char *nuvolta_1671_hall_s_prop(void)
{
	const struct mca_hwid *hwid = mca_get_hwid_info();

	if (hwid && hwid->platform_version == 3 && hwid->build_version <= 1 &&
	    hwid->minor_version < 2)
		return "hall-s-p1-int";

	return "hall-s-int";
}

static int nuvolta_1671_parse_dt(struct nuvolta_1671_chg *chip)
{
	struct device_node *node = chip->dev->of_node;
	int ret;

	if (!node) {
		mca_log_err("No DT data Failing Probe\n");
		return -EINVAL;
	}

	(void)mca_parse_dts_u32(node, "project_vendor", &chip->project_vendor,
				0);
	(void)mca_parse_dts_u32(node, "support-hall", &chip->support_hall, 0);
	(void)mca_parse_dts_u32(node, "rx_role", &chip->rx_role, 0);
	(void)mca_parse_dts_u32(node, "fw_version_index",
				&chip->fw_version_index, 0);
	mca_log_err("project_vendor = %d, support-hall = %d, fw_version_index = %d",
		    chip->project_vendor, chip->support_hall,
		    chip->fw_version_index);

	if (nuvolta_1671_parse_fw_fod_data(chip))
		mca_log_err("failed to parse fod params\n");

	chip->nu1671_pinctrl = devm_pinctrl_get(chip->dev);
	if (IS_ERR_OR_NULL(chip->nu1671_pinctrl)) {
		mca_log_err("failed to get nu1671_pinctrl\n");
		return -EINVAL;
	}

	chip->enable_gpio = of_get_named_gpio(node, "sleep-rx-gpio", 0);
	if (!gpio_is_valid(chip->enable_gpio)) {
		mca_log_err("fail_enable_gpio %d\n", chip->enable_gpio);
		return -EINVAL;
	}
	ret = gpio_request(chip->enable_gpio, "rx-enable-gpio");
	if (ret) {
		mca_log_err("request enable gpio [%d] fail\n",
			    chip->enable_gpio);
	} else {
		ret = gpio_direction_output(chip->enable_gpio, 0);
		if (ret)
			mca_log_err("set direction for enable gpio [%d] fail\n",
				    chip->enable_gpio);
	}

	chip->irq_gpio = of_get_named_gpio(node, "rx-int", 0);
	if (!gpio_is_valid(chip->irq_gpio)) {
		mca_log_err("fail_irq_gpio %d\n", chip->irq_gpio);
		return -EINVAL;
	}

	if (chip->support_hall) {
		chip->hall_n_int_gpio = of_get_named_gpio(node, "hall-n-int", 0);
		if (!gpio_is_valid(chip->hall_n_int_gpio)) {
			mca_log_err("fail_hall-n-int %d\n",
				    chip->hall_n_int_gpio);
			return -EINVAL;
		}

		chip->hall_s_int_gpio =
			of_get_named_gpio(node, nuvolta_1671_hall_s_prop(), 0);
		if (!gpio_is_valid(chip->hall_s_int_gpio)) {
			mca_log_err("fail_hall-s-int %d\n",
				    chip->hall_s_int_gpio);
			return -EINVAL;
		}
	}

	chip->power_good_gpio = of_get_named_gpio(node, "pwr-det-int", 0);
	if (!gpio_is_valid(chip->power_good_gpio)) {
		mca_log_err("fail_power_good_gpio %d\n", chip->power_good_gpio);
		return -EINVAL;
	}

	return 0;
}

static int nuvolta_1671_gpio_to_irq(int gpio, int *irq, const char *what)
{
	if (!gpio_is_valid(gpio)) {
		mca_log_err("%s gpio not provided\n", what);
		return -EINVAL;
	}

	*irq = gpio_to_irq(gpio);
	if (*irq < 0) {
		mca_log_err("gpio_to_%s Fail!\n", what);
		*irq = 0;
		return -EINVAL;
	}

	return 0;
}

static void nuvolta_rx1671_gpio_init(struct nuvolta_1671_chg *chip)
{
	if (nuvolta_1671_gpio_to_irq(chip->irq_gpio, &chip->irq, "irq"))
		return;
	if (nuvolta_1671_gpio_to_irq(chip->power_good_gpio,
				     &chip->power_good_irq, "power_good"))
		return;
	if (!chip->support_hall)
		return;
	if (nuvolta_1671_gpio_to_irq(chip->hall_n_int_gpio,
				     &chip->hall_n_int_irq, "hall_n_int"))
		return;
	nuvolta_1671_gpio_to_irq(chip->hall_s_int_gpio, &chip->hall_s_int_irq,
				 "hall_s_int");
}

static void nuvolta_1671_request_irq(struct nuvolta_1671_chg *chip, int irq,
				     irq_handler_t handler,
				     unsigned long flags, const char *name)
{
	int ret;

	if (!irq)
		return;

	ret = devm_request_threaded_irq(chip->dev, irq, NULL, handler, flags,
					name, chip);
	if (ret)
		mca_log_err("Failed %s = %d ret = %d\n", name, irq, ret);

	enable_irq_wake(irq);
}

static int nuvolta_1671_shutdown_cb(struct notifier_block *nb,
				    unsigned long code, void *unused)
{
	struct nuvolta_1671_chg *chip =
		container_of(nb, struct nuvolta_1671_chg, shutdown_notifier);

	if (code == SYS_POWER_OFF || code == SYS_RESTART) {
		mca_log_info("start adsp shutdown\n");
		if (chip->proc_data.power_good_flag) {
			nuvolta_1671_vote_enable(chip, NU1671_EN_VOTE_SELF,
						 false);
			msleep(150);
			nuvolta_1671_vote_enable(chip, NU1671_EN_VOTE_SELF,
						 true);
		}
	}

	return NOTIFY_DONE;
}

static int nuvolta_1671_probe(struct i2c_client *client)
{
	struct nuvolta_1671_chg *chip;
	int ret;

	mca_log_err("nu1671 %s probe start!\n", "1.0.0_G");

	chip = devm_kzalloc(&client->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->wls_fw_data = devm_kzalloc(&client->dev,
					 sizeof(*chip->wls_fw_data), GFP_KERNEL);
	if (!chip->wls_fw_data)
		return -ENOMEM;

	chip->regmap =
		devm_regmap_init_i2c(client, &nuvolta_1671_regmap_config);
	if (IS_ERR(chip->regmap)) {
		mca_log_err("failed to allocate register map\n");
		return PTR_ERR(chip->regmap);
	}

	chip->client = client;
	chip->dev = &client->dev;
	chip->enable_status = true;
	chip->irq_gpio = -EINVAL;
	chip->power_good_gpio = -EINVAL;
	chip->enable_gpio = -EINVAL;
	chip->hall_n_int_gpio = -EINVAL;
	chip->hall_s_int_gpio = -EINVAL;

	device_init_wakeup(&client->dev, true);
	i2c_set_clientdata(client, chip);

	mutex_init(&chip->i2c_lock);
	mutex_init(&chip->wireless_chg_int_lock);
	mutex_init(&chip->wireless_enable_lock);
	mutex_init(&chip->data_transfer_lock);

	nuvolta_1671_parse_dt(chip);
	nuvolta_rx1671_gpio_init(chip);

	INIT_DELAYED_WORK(&chip->interrupt_work, nuvolta_1671_interrupt_work);
	INIT_DELAYED_WORK(&chip->pg_detect_work, nuvolta_1671_pg_det_work);
	INIT_DELAYED_WORK(&chip->init_detect_work,
			  nuvolta_1671_init_detect_work);
	INIT_DELAYED_WORK(&chip->hall_interrupt_work,
			  nuvolta_1671_hall_interrupt_work);

	chip->shutdown_notifier.notifier_call = nuvolta_1671_shutdown_cb;
	chip->shutdown_notifier.priority = 255;
	register_reboot_notifier(&chip->shutdown_notifier);

	nuvolta_1671_request_irq(chip, chip->irq,
				 nuvolta_1671_interrupt_handler,
				 IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
				 "nuvolta_1671_chg_stat_irq");
	nuvolta_1671_request_irq(chip, chip->power_good_irq,
				 nuvolta_1671_power_good_handler,
				 IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING |
					 IRQF_ONESHOT,
				 "nuvolta_1671_power_good_irq");
	if (chip->support_hall) {
		nuvolta_1671_request_irq(chip, chip->hall_n_int_irq,
					 nuvolta_1671_hall_interrupt_handler,
					 IRQF_TRIGGER_FALLING |
						 IRQF_TRIGGER_RISING |
						 IRQF_ONESHOT,
					 "nuvolta_1671_hall_n_int_irq");
		nuvolta_1671_request_irq(chip, chip->hall_s_int_irq,
					 nuvolta_1671_hall_interrupt_handler,
					 IRQF_TRIGGER_FALLING |
						 IRQF_TRIGGER_RISING |
						 IRQF_ONESHOT,
					 "nuvolta_1671_hall_s_int_irq");
	}

	ret = platform_class_wireless_register_ops(chip->rx_role, chip,
						   &nuvolta_1671_wls_ops);
	if (ret) {
		mca_log_err("register ops fail\n");
		goto err_dev;
	}

	schedule_delayed_work(&chip->init_detect_work, msecs_to_jiffies(1000));
	if (chip->support_hall)
		schedule_delayed_work(&chip->hall_interrupt_work,
				      msecs_to_jiffies(1200));

	mca_log_err("nuvolta 1671 probe success!\n");
	return 0;

err_dev:
	unregister_reboot_notifier(&chip->shutdown_notifier);
	if (gpio_is_valid(chip->enable_gpio))
		gpio_free(chip->enable_gpio);
	mca_log_err("nuvolta 1671 probe failed\n");
	return ret;
}

static int nuvolta_1671_suspend(struct device *dev)
{
	struct nuvolta_1671_chg *chip = dev_get_drvdata(dev);

	return chip->irq ? enable_irq_wake(chip->irq) : 0;
}

static int nuvolta_1671_resume(struct device *dev)
{
	struct nuvolta_1671_chg *chip = dev_get_drvdata(dev);

	return chip->irq ? disable_irq_wake(chip->irq) : 0;
}

static void nuvolta_1671_remove(struct i2c_client *client)
{
	struct nuvolta_1671_chg *chip = i2c_get_clientdata(client);

	unregister_reboot_notifier(&chip->shutdown_notifier);
	cancel_delayed_work_sync(&chip->init_detect_work);
	cancel_delayed_work_sync(&chip->pg_detect_work);
	cancel_delayed_work_sync(&chip->interrupt_work);
	cancel_delayed_work_sync(&chip->hall_interrupt_work);

	if (gpio_is_valid(chip->enable_gpio))
		gpio_free(chip->enable_gpio);

	kfree(chip->wls_debug_all_fod_params);
	mutex_destroy(&chip->i2c_lock);
	mutex_destroy(&chip->wireless_chg_int_lock);
	mutex_destroy(&chip->wireless_enable_lock);
	mutex_destroy(&chip->data_transfer_lock);
}

static void nuvolta_1671_shutdown(struct i2c_client *client)
{
	struct nuvolta_1671_chg *chip = i2c_get_clientdata(client);

	mutex_lock(&chip->i2c_lock);
	chip->shutdown_flag = true;
	mutex_unlock(&chip->i2c_lock);
}

static const struct dev_pm_ops nuvolta_1671_pm_ops = {
	.suspend = nuvolta_1671_suspend,
	.resume = nuvolta_1671_resume,
};

static const struct of_device_id nuvolta_1671_of_match[] = {
	{
		.compatible = "fuda,nu1671",
	},
	{},
};

static const struct i2c_device_id nuvolta_1671_id[] = {
	{ "nuvolta_1671", 0 },
	{},
};

static struct i2c_driver nuvolta_1671_i2c_driver = {
	.driver		= {
		.name = "nuvolta_1671",
		.owner = THIS_MODULE,
		.of_match_table = nuvolta_1671_of_match,
		.pm = &nuvolta_1671_pm_ops,
	},
	.probe		= nuvolta_1671_probe,
	.remove		= nuvolta_1671_remove,
	.shutdown	= nuvolta_1671_shutdown,
	.id_table	= nuvolta_1671_id,
};

module_i2c_driver(nuvolta_1671_i2c_driver);

MODULE_DESCRIPTION("nuvolta wireless charge driver");
MODULE_AUTHOR("wuliyang@xiaomi.com");
MODULE_LICENSE("GPL v2");
