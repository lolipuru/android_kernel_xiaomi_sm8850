def register_modules(registry):
    registry.register(
        name = "drivers/power/supply/mca/mca_hardware_monitor/mca_ibat_ocp_monitor",
        out = "mca_ibat_ocp_monitor.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_hardware_monitor/inc/mca_ibat_ocp_monitor.h",
            "drivers/power/supply/mca/mca_hardware_monitor/mca_ibat_ocp_monitor.c",
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
            "drivers/power/supply/mca/mca_common/mca_workqueue",
            "drivers/power/supply/mca/mca_platform/mca_platform_buckchg_class",
            "drivers/misc/hwid",
            "drivers/power/supply/mca/mca_common/mca_charge_mievent",
            "drivers/power/supply/mca/mca_common/mca_parse_dts",
            "drivers/power/supply/mca/mca_platform/mca_platform_fg_ic_ops",
            "drivers/power/supply/mca/mca_platform/mca_platform_loadsw_class",
            "drivers/power/supply/mca/mca_platform/mca_platform_wireless_class",
            "drivers/power/supply/mca/mca_strategy/strategy_fg/mca_strategy_fg_class",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_hardware_monitor/mca_vbat_ovp_monitor",
        out = "mca_vbat_ovp_monitor.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_hardware_monitor/inc/mca_vbat_ovp_monitor.h",
            "drivers/power/supply/mca/mca_hardware_monitor/mca_vbat_ovp_monitor.c",
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
            "drivers/power/supply/mca/mca_common/mca_workqueue",
            "drivers/power/supply/mca/mca_platform/mca_platform_buckchg_class",
            "drivers/misc/hwid",
            "drivers/power/supply/mca/mca_common/mca_hwid",
            "drivers/power/supply/mca/mca_common/mca_parse_dts",
            "drivers/power/supply/mca/mca_platform/mca_platform_wireless_class",
            "drivers/power/supply/mca/mca_strategy/strategy_fg/mca_strategy_fg_class",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_hardware_monitor/mca_lpd_detect",
        out = "mca_lpd_detect.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_hardware_monitor/inc/mca_lpd_detect.h",
            "drivers/power/supply/mca/mca_hardware_monitor/mca_lpd_detect.c",
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
            "drivers/power/supply/mca/mca_common/mca_workqueue",
            "drivers/power/supply/mca/mca_common/mca_charge_mievent",
            "drivers/power/supply/mca/mca_platform/mca_platform_buckchg_class",
            "drivers/misc/hwid",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_hardware_monitor/mca_path_control",
        out = "mca_path_control.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_hardware_monitor/inc/mca_path_control.h",
            "drivers/power/supply/mca/mca_hardware_monitor/mca_path_control.c",
        ],
        hdrs = [
            "drivers/power/supply/mca/include/mca/hardware/hw_path_control.h",
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
            "drivers/power/supply/mca/mca_common/mca_workqueue",
            "drivers/power/supply/mca/mca_platform/mca_platform_buckchg_class",
            "drivers/misc/hwid",
            "drivers/power/supply/mca/mca_common/mca_parse_dts",
            "drivers/power/supply/mca/mca_platform/mca_platform_cp_class",
            "drivers/power/supply/mca/mca_platform/mca_platform_wireless_class",
            "drivers/power/supply/mca/mca_strategy/strategy_wireless/mca_wireless_revchg",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_hardware_monitor/mca_bmd",
        out = "mca_bmd.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_hardware_monitor/inc/mca_bmd.h",
            "drivers/power/supply/mca/mca_hardware_monitor/mca_bmd.c",
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
            "drivers/power/supply/mca/mca_common/mca_workqueue",
            "drivers/power/supply/mca/mca_platform/mca_platform_buckchg_class",
            "drivers/misc/hwid",
            "drivers/power/supply/mca/mca_common/mca_charge_mievent",
            "drivers/power/supply/mca/mca_common/mca_parse_dts",
            "drivers/power/supply/mca/mca_platform/mca_platform_wireless_class",
            "drivers/power/supply/mca/mca_strategy/mca_strategy_class",
            "drivers/power/supply/mca/mca_strategy/strategy_fg/mca_strategy_fg_class",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_hardware_monitor/mca_connector_antiburn",
        out = "mca_connector_antiburn.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_hardware_monitor/inc/mca_connector_antiburn.h",
            "drivers/power/supply/mca/mca_hardware_monitor/mca_connector_antiburn.c",
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
            "drivers/power/supply/mca/mca_common/mca_workqueue",
            "drivers/power/supply/mca/mca_common/mca_hwid",
            "drivers/power/supply/mca/mca_platform/mca_platform_buckchg_class",
            "drivers/misc/hwid",
            "drivers/power/supply/mca/mca_common/mca_charge_mievent",
            "drivers/power/supply/mca/mca_common/mca_parse_dts",
            "drivers/power/supply/mca/mca_protocol/mca_protocol_class",
            "drivers/power/supply/mca/mca_protocol/protocol_pd/mca_protocol_pd_class",
        ],
    )
