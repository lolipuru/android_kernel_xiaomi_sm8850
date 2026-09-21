#ifndef __MCA_BMD_H__
#define __MCA_BMD_H__

enum batt_connectot {
	MASTER_BTB,
	SLAVE_BTB,
	MAX_BTB,
};

bool mca_bmd_is_batt_missing(void);
void mca_bmd_force_detection(void);

#endif /*__HW_MCA_BMD_H__*/