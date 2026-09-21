def register_modules(registry):
    registry.register(
        name = "drivers/power/supply/mca/mca_strategy/mca_strategy_class",
        out = "mca_strategy_class.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_strategy/mca_strategy_class.c",
        ],
        hdrs = [
            "drivers/power/supply/mca/include/mca/strategy/strategy_class.h",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_strategy/strategy_fg/mca_strategy_fg_class",
        out = "mca_strategy_fg_class.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_strategy/strategy_fg/mca_strategy_fg_class.c",
        ],
        hdrs = [
            "drivers/power/supply/mca/include/mca/strategy/strategy_fg_class.h",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_strategy/strategy_fg/mca_soc_limit",
        out = "mca_soc_limit.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_strategy/strategy_fg/inc/mca_soc_limit.h",
            "drivers/power/supply/mca/mca_strategy/strategy_fg/mca_soc_limit.c",
        ],
        hdrs = [
            "drivers/power/supply/mca/include/mca/strategy/strategy_soc_limit.h",
            "drivers/power/supply/mca/include/mca/strategy/strategy_soc_limit_stepper.h",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log",
            "drivers/power/supply/mca/mca_common/mca_event",
            "drivers/power/supply/mca/mca_strategy/strategy_fg/mca_strategy_fg_class",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_strategy/strategy_fg/mca_strategy_fg_comp",
        out = "mca_strategy_fg_comp.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_strategy/strategy_fg/inc/mca_battery_shutdown.h",
            "drivers/power/supply/mca/mca_strategy/strategy_fg/inc/mca_soc_smooth.h",
            "drivers/power/supply/mca/mca_strategy/strategy_fg/inc/strategy_fg.h",
            "drivers/power/supply/mca/mca_strategy/strategy_fg/inc/xm_battery_auth.h",
            "drivers/power/supply/mca/mca_strategy/strategy_fg/mca_battery_shutdown.c",
            "drivers/power/supply/mca/mca_strategy/strategy_fg/mca_soc_smooth.c",
            "drivers/power/supply/mca/mca_strategy/strategy_fg/mca_strategy_fg.c",
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
            "drivers/power/supply/mca/mca_common/mca_parse_dts",
            "drivers/power/supply/mca/mca_common/mca_workqueue",
            "drivers/power/supply/mca/mca_common",
            "drivers/power/supply/mca/mca_common/mca_hwid",
            "drivers/power/supply/mca/mca_common/mca_charge_mievent",
            "drivers/power/supply/mca/mca_platform/mca_platform_fg_ic_ops",
            "drivers/power/supply/mca/mca_platform/mca_platform_buckchg_class",
            "drivers/power/supply/mca/mca_strategy/mca_strategy_class",
            "drivers/power/supply/mca/mca_strategy/strategy_fg/mca_strategy_fg_class",
            "drivers/power/supply/mca/mca_strategy/strategy_fg/mca_soc_limit",
            "drivers/misc/hwid",
            "drivers/power/supply/mca/mca_common/mca_qcom_smem",
            "drivers/power/supply/mca/mca_platform/mca_platform_loadsw_class",
            "drivers/power/supply/mca/mca_protocol/mca_protocol_class",
            "drivers/power/supply/mca/mca_protocol/protocol_pd/mca_protocol_pd_class",
            "drivers/power/supply/mca/mca_shared_memory/charger_partition",
            "drivers/power/supply/mca/mca_smart_charge/mca_smart_charge",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_strategy/strategy_charger/mca_strategy_quickchg",
        out = "mca_strategy_quickchg.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_strategy/strategy_charger/inc/mca_quick_charger.h",
            "drivers/power/supply/mca/mca_strategy/strategy_charger/mca_strategy_quickchg.c",
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
            "drivers/power/supply/mca/mca_common/mca_parse_dts",
            "drivers/power/supply/mca/mca_common/mca_workqueue",
            "drivers/power/supply/mca/mca_common",
            "drivers/power/supply/mca/mca_common/mca_hwid",
            "drivers/power/supply/mca/mca_common/mca_charge_mievent",
            "drivers/power/supply/mca/mca_platform/mca_platform_buckchg_class",
            "drivers/power/supply/mca/mca_platform/mca_platform_cp_class",
            "drivers/power/supply/mca/mca_strategy/mca_strategy_class",
            "drivers/misc/hwid",
            "drivers/power/supply/mca/mca_common/mca_charge_interface",
            "drivers/power/supply/mca/mca_platform/mca_platform_fg_ic_ops",
            "drivers/power/supply/mca/mca_platform/mca_platform_loadsw_class",
            "drivers/power/supply/mca/mca_protocol/mca_protocol_class",
            "drivers/power/supply/mca/mca_protocol/protocol_pd/mca_protocol_pd_class",
            "drivers/power/supply/mca/mca_protocol/protocol_qc/mca_protocol_qc_class",
            "drivers/power/supply/mca/mca_smart_charge/mca_smart_charge",
            "drivers/power/supply/mca/mca_strategy/strategy_fg/mca_strategy_fg_class",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_strategy/strategy_charger/mca_strategy_buckchg",
        out = "mca_strategy_buckchg.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_strategy/strategy_charger/inc/mca_strategy_buckchg.h",
            "drivers/power/supply/mca/mca_strategy/strategy_charger/mca_strategy_buckchg.c",
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
            "drivers/power/supply/mca/mca_common/mca_parse_dts",
            "drivers/power/supply/mca/mca_common/mca_workqueue",
            "drivers/power/supply/mca/mca_common",
            "drivers/power/supply/mca/mca_common/mca_hwid",
            "drivers/power/supply/mca/mca_platform/mca_platform_buckchg_class",
            "drivers/power/supply/mca/mca_strategy/mca_strategy_class",
            "drivers/misc/hwid",
            "drivers/power/supply/mca/mca_common/mca_charge_interface",
            "drivers/power/supply/mca/mca_common/mca_charge_mievent",
            "drivers/power/supply/mca/mca_common/mca_qcom_smem",
            "drivers/power/supply/mca/mca_platform/mca_platform_cp_class",
            "drivers/power/supply/mca/mca_platform/mca_platform_fg_ic_ops",
            "drivers/power/supply/mca/mca_platform/mca_platform_loadsw_class",
            "drivers/power/supply/mca/mca_protocol/mca_protocol_class",
            "drivers/power/supply/mca/mca_protocol/protocol_pd/mca_protocol_pd_class",
            "drivers/power/supply/mca/mca_smart_charge/mca_smart_charge",
            "drivers/power/supply/mca/mca_strategy/strategy_fg/mca_strategy_fg_class",
            "drivers/power/supply/mca/mca_strategy/strategy_wireless/mca_wireless_revchg",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_strategy/strategy_charger/mca_charger_thermal",
        out = "mca_charger_thermal.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_strategy/strategy_charger/inc/mca_charger_thermal.h",
            "drivers/power/supply/mca/mca_strategy/strategy_charger/mca_charger_thermal.c",
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
            "drivers/power/supply/mca/mca_common/mca_parse_dts",
            "drivers/power/supply/mca/mca_common/mca_workqueue",
            "drivers/power/supply/mca/mca_common",
            "drivers/power/supply/mca/mca_common/mca_hwid",
            "drivers/power/supply/mca/mca_strategy/mca_strategy_class",
            "drivers/misc/hwid",
            "drivers/power/supply/mca/mca_common/mca_qcom_smem",
            "drivers/power/supply/mca/mca_platform/mca_platform_fg_ic_ops",
            "drivers/power/supply/mca/mca_protocol/mca_protocol_class",
            "drivers/power/supply/mca/mca_smart_charge/mca_smart_charge",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_strategy/strategy_charger/mca_buckchg_jeita",
        out = "mca_buckchg_jeita.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_strategy/strategy_charger/inc/mca_buckchg_jeita.h",
            "drivers/power/supply/mca/mca_strategy/strategy_charger/mca_buckchg_jeita.c",
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
            "drivers/power/supply/mca/mca_common/mca_parse_dts",
            "drivers/power/supply/mca/mca_common/mca_workqueue",
            "drivers/power/supply/mca/mca_common",
            "drivers/power/supply/mca/mca_common/mca_hwid",
            "drivers/power/supply/mca/mca_strategy/mca_strategy_class",
            "drivers/misc/hwid",
            "drivers/power/supply/mca/mca_platform/mca_platform_fg_ic_ops",
            "drivers/power/supply/mca/mca_platform/mca_platform_loadsw_class",
            "drivers/power/supply/mca/mca_smart_charge/mca_smart_charge",
            "drivers/power/supply/mca/mca_strategy/strategy_fg/mca_strategy_fg_class",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_strategy/strategy_wireless/mca_basic_wireless",
        out = "mca_basic_wireless.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_strategy/strategy_wireless/inc/mca_basic_wireless.h",
            "drivers/power/supply/mca/mca_strategy/strategy_wireless/mca_basic_wireless.c",
        ],
        hdrs = [
            "drivers/power/supply/mca/include/mca/strategy/strategy_wireless_class.h",
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
            "drivers/power/supply/mca/mca_common/mca_parse_dts",
            "drivers/power/supply/mca/mca_common/mca_workqueue",
            "drivers/power/supply/mca/mca_common",
            "drivers/power/supply/mca/mca_common/mca_hwid",
            "drivers/power/supply/mca/mca_platform/mca_platform_wireless_class",
            "drivers/power/supply/mca/mca_strategy/mca_strategy_class",
            "drivers/misc/hwid",
            "drivers/power/supply/mca/mca_common/mca_charge_interface",
            "drivers/power/supply/mca/mca_common/mca_charge_mievent",
            "drivers/power/supply/mca/mca_platform/mca_platform_buckchg_class",
            "drivers/power/supply/mca/mca_platform/mca_platform_cp_class",
            "drivers/power/supply/mca/mca_platform/mca_platform_loadsw_class",
            "drivers/power/supply/mca/mca_protocol/protocol_pd/mca_protocol_pd_class",
            "drivers/power/supply/mca/mca_smart_charge/mca_smart_charge",
            "drivers/power/supply/mca/mca_strategy/strategy_fg/mca_strategy_fg_class",
            "drivers/power/supply/mca/mca_strategy/strategy_wireless/mca_wireless_revchg",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_strategy/strategy_wireless/mca_quick_wireless",
        out = "mca_quick_wireless.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_strategy/strategy_wireless/inc/mca_quick_wireless.h",
            "drivers/power/supply/mca/mca_strategy/strategy_wireless/mca_quick_wireless.c",
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
            "drivers/power/supply/mca/mca_common/mca_parse_dts",
            "drivers/power/supply/mca/mca_common/mca_workqueue",
            "drivers/power/supply/mca/mca_common",
            "drivers/power/supply/mca/mca_common/mca_hwid",
            "drivers/power/supply/mca/mca_platform/mca_platform_wireless_class",
            "drivers/power/supply/mca/mca_strategy/mca_strategy_class",
            "drivers/power/supply/mca/mca_strategy/strategy_wireless/mca_basic_wireless",
            "drivers/misc/hwid",
            "drivers/power/supply/mca/mca_common/mca_charge_interface",
            "drivers/power/supply/mca/mca_platform/mca_platform_buckchg_class",
            "drivers/power/supply/mca/mca_platform/mca_platform_cp_class",
            "drivers/power/supply/mca/mca_platform/mca_platform_fg_ic_ops",
            "drivers/power/supply/mca/mca_platform/mca_platform_loadsw_class",
            "drivers/power/supply/mca/mca_smart_charge/mca_smart_charge",
            "drivers/power/supply/mca/mca_strategy/strategy_fg/mca_strategy_fg_class",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_strategy/strategy_wireless/mca_wireless_revchg",
        out = "mca_wireless_revchg.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_strategy/strategy_wireless/inc/mca_wireless_revchg.h",
            "drivers/power/supply/mca/mca_strategy/strategy_wireless/mca_wireless_revchg.c",
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
            "drivers/power/supply/mca/mca_common/mca_parse_dts",
            "drivers/power/supply/mca/mca_common/mca_workqueue",
            "drivers/power/supply/mca/mca_common",
            "drivers/power/supply/mca/mca_common/mca_hwid",
            "drivers/power/supply/mca/mca_platform/mca_platform_wireless_class",
            "drivers/power/supply/mca/mca_strategy/mca_strategy_class",
            "drivers/misc/hwid",
            "drivers/power/supply/mca/mca_common/mca_charge_mievent",
            "drivers/power/supply/mca/mca_platform/mca_platform_buckchg_class",
            "drivers/power/supply/mca/mca_platform/mca_platform_cp_class",
            "drivers/power/supply/mca/mca_protocol/mca_protocol_class",
            "drivers/power/supply/mca/mca_strategy/strategy_fg/mca_strategy_fg_class",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_strategy/strategy_pd/mca_pd_auth",
        out = "mca_pd_auth.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_strategy/strategy_pd/pd_auth.h",
            "drivers/power/supply/mca/mca_strategy/strategy_pd/mca_pd_auth.c",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_sysfs",
            "drivers/power/supply/mca/mca_common/mca_event",
            "drivers/power/supply/mca/mca_common/mca_log",
            "drivers/power/supply/mca/mca_common/mca_charge_mievent",
            "drivers/power/supply/mca/mca_common/mca_charge_interface",
            "drivers/power/supply/mca/mca_platform/mca_platform_buckchg_class",
            "drivers/power/supply/mca/mca_protocol/protocol_pd/mca_protocol_pd_class",
        ],
    )
