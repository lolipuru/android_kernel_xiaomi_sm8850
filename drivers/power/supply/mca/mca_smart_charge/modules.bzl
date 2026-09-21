def register_modules(registry):
    registry.register(
        name = "drivers/power/supply/mca/mca_smart_charge/mca_smart_charge",
        out = "mca_smart_charge.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_smart_charge/inc/mca_smart_charge.h",
            "drivers/power/supply/mca/mca_smart_charge/mca_smart_charge.c",
        ],
        hdrs = [
            "drivers/power/supply/mca/include/mca/smartchg/smart_chg_class.h",
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
            "drivers/power/supply/mca/mca_common",
            "drivers/power/supply/mca/mca_common/mca_hwid",
            "drivers/power/supply/mca/mca_common/mca_charge_mievent",
            "drivers/power/supply/mca/mca_protocol/mca_protocol_class",
            "drivers/power/supply/mca/mca_strategy/mca_strategy_class",
            "drivers/power/supply/mca/mca_strategy/strategy_fg/mca_strategy_fg_class",
            "drivers/power/supply/mca/mca_strategy/strategy_fg/mca_soc_limit",
            "drivers/power/supply/mca/mca_shared_memory/charger_partition",
            "drivers/misc/hwid",
        ],
    )
