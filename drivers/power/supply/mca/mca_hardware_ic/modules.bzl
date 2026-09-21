def register_modules(registry):
    registry.register(
        name = "drivers/power/supply/mca/mca_hardware_ic/boost_ic/hl7603/hl7603",
        out = "hl7603.ko",
        config = "CONFIG_MCA_EXTERNAL_BOOST",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_hardware_ic/boost_ic/hl7603/inc/hl7603.h",
            "drivers/power/supply/mca/mca_hardware_ic/boost_ic/hl7603/hl7603.c",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log",
            "drivers/power/supply/mca/mca_common/mca_event",
            "drivers/power/supply/mca/mca_common/mca_parse_dts",
            "drivers/power/supply/mca/mca_common/mca_sysfs",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_hardware_ic/wireless_ic/nuvolta_1652/nuvolta_1652",
        out = "nuvolta_1652.ko",
        config = "CONFIG_MCA_EXTERNAL_WIRELESS",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_hardware_ic/wireless_ic/nuvolta_1652/inc/fw_update_nu1652.h",
            "drivers/power/supply/mca/mca_hardware_ic/wireless_ic/nuvolta_1652/inc/fw_update_nu1661.h",
            "drivers/power/supply/mca/mca_hardware_ic/wireless_ic/nuvolta_1652/inc/fw_update_nu1665.h",
            "drivers/power/supply/mca/mca_hardware_ic/wireless_ic/nuvolta_1652/inc/nuvolta_1652.h",
            "drivers/power/supply/mca/mca_hardware_ic/wireless_ic/nuvolta_1652/nuvolta_1652.c",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_sysfs",
            "drivers/power/supply/mca/mca_common/mca_event",
            "drivers/power/supply/mca/mca_common/mca_log",
            "drivers/power/supply/mca/mca_platform/mca_platform_wireless_class",
            "drivers/power/supply/mca/mca_common/mca_adsp_glink",
            "drivers/power/supply/mca/mca_common/mca_parse_dts",
            "drivers/power/supply/mca/mca_strategy/mca_strategy_class",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_hardware_ic/wireless_ic/sc96281/sc96281_comp",
        out = "sc96281_comp.ko",
        config = "CONFIG_MCA_EXTERNAL_WIRELESS_SC96281",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_hardware_ic/wireless_ic/sc96281/inc/sc96281_firmware.h",
            "drivers/power/supply/mca/mca_hardware_ic/wireless_ic/sc96281/inc/sc96281_mtp_program.h",
            "drivers/power/supply/mca/mca_hardware_ic/wireless_ic/sc96281/inc/sc96281_pgm.h",
            "drivers/power/supply/mca/mca_hardware_ic/wireless_ic/sc96281/inc/sc96281_reg.h",
            "drivers/power/supply/mca/mca_hardware_ic/wireless_ic/sc96281/sc96281_charger.c",
            "drivers/power/supply/mca/mca_hardware_ic/wireless_ic/sc96281/sc96281_mtp_program.c",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_sysfs",
            "drivers/power/supply/mca/mca_common/mca_event",
            "drivers/power/supply/mca/mca_common/mca_log",
            "drivers/power/supply/mca/mca_platform/mca_platform_wireless_class",
            "drivers/power/supply/mca/mca_common/mca_adsp_glink",
            "drivers/power/supply/mca/mca_common/mca_parse_dts",
            "drivers/power/supply/mca/mca_strategy/mca_strategy_class",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_hardware_ic/charge_pump_ic/sc8541/sc8541",
        out = "sc8541.ko",
        config = "CONFIG_MCA_EXTERNAL_CP_SC8541",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_hardware_ic/charge_pump_ic/sc8541/inc/sc8541_reg.h",
            "drivers/power/supply/mca/mca_hardware_ic/charge_pump_ic/sc8541/sc8541.c",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log",
            "drivers/power/supply/mca/mca_platform/mca_platform_cp_class",
            "drivers/power/supply/mca/mca_common/mca_charge_mievent",
            "drivers/power/supply/mca/mca_common/mca_event",
            "drivers/power/supply/mca/mca_common/mca_parse_dts",
            "drivers/power/supply/mca/mca_common/mca_sysfs",
            "drivers/power/supply/mca/mca_platform/mca_platform_wireless_class",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_hardware_ic/charge_pump_ic/sc8581/sc8581",
        out = "sc8581.ko",
        config = "CONFIG_MCA_EXTERNAL_CP",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_hardware_ic/charge_pump_ic/sc8581/inc/sc8581.h",
            "drivers/power/supply/mca/mca_hardware_ic/charge_pump_ic/sc8581/inc/sc8581_reg.h",
            "drivers/power/supply/mca/mca_hardware_ic/charge_pump_ic/sc8581/sc8581.c",
        ],
        includes = [
            "drivers/power/supply/mca/include",
            "drivers/misc/hwid",
        ],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log",
            "drivers/power/supply/mca/mca_common/mca_hwid",
            "drivers/power/supply/mca/mca_platform/mca_platform_cp_class",
            "drivers/misc/hwid",
            "drivers/power/supply/mca/mca_common/mca_event",
            "drivers/power/supply/mca/mca_common/mca_parse_dts",
            "drivers/power/supply/mca/mca_common/mca_sysfs",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_hardware_ic/fuelgauge_ic/bq27z561/bq27z561",
        out = "bq27z561.ko",
        config = "CONFIG_MCA_EXTERNAL_FG",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_hardware_ic/fuelgauge_ic/bq27z561/inc/bq27z561.h",
            "drivers/power/supply/mca/mca_hardware_ic/fuelgauge_ic/bq27z561/inc/bq27z561_nfg1000_fw.h",
            "drivers/power/supply/mca/mca_hardware_ic/fuelgauge_ic/bq27z561/bq27z561.c",
        ],
        includes = [
            "drivers/power/supply/mca/include",
            "drivers/misc/hwid",
        ],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_sysfs",
            "drivers/power/supply/mca/mca_common/mca_event",
            "drivers/power/supply/mca/mca_common/mca_log",
            "drivers/power/supply/mca/mca_common/mca_hwid",
            "drivers/power/supply/mca/mca_platform/mca_platform_fg_ic_ops",
            "drivers/misc/hwid",
            "drivers/power/supply/mca/mca_common/mca_parse_dts",
            "drivers/power/supply/mca/mca_strategy/mca_strategy_class",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_hardware_ic/load_switch_ic/sc7601/sc7601",
        out = "sc7601.ko",
        config = "CONFIG_MCA_EXTERNAL_LOADSWITCH",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_hardware_ic/load_switch_ic/sc7601/inc/sc7601_reg.h",
            "drivers/power/supply/mca/mca_hardware_ic/load_switch_ic/sc7601/sc7601.c",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log",
            "drivers/power/supply/mca/mca_platform/mca_platform_loadsw_class",
            "drivers/power/supply/mca/mca_common/mca_parse_dts",
            "drivers/power/supply/mca/mca_common/mca_sysfs",
        ],
    )
