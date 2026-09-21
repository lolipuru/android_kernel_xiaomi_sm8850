def register_modules(registry):
    registry.register(
        name = "drivers/power/supply/mca/mca_business/business_charger/mca_business_charger_comp",
        out = "mca_business_charger_comp.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_business/business_charger/inc/mca_business_charger.h",
            "drivers/power/supply/mca/mca_business/business_charger/inc/mca_charger_usb_psy.h",
            "drivers/power/supply/mca/mca_business/business_charger/inc/mca_charger_wireless_psy.h",
            "drivers/power/supply/mca/mca_business/business_charger/mca_business_charger.c",
            "drivers/power/supply/mca/mca_business/business_charger/mca_charger_usb_psy.c",
            "drivers/power/supply/mca/mca_business/business_charger/mca_charger_wireless_psy.c",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_sysfs",
            "drivers/power/supply/mca/mca_common/mca_event",
            "drivers/power/supply/mca/mca_common/mca_log",
            "drivers/power/supply/mca/mca_common/mca_parse_dts",
            "drivers/power/supply/mca/mca_common/mca_charge_mievent",
            "drivers/power/supply/mca/mca_hardware_monitor/mca_connector_antiburn",
            "drivers/power/supply/mca/mca_hardware_monitor/mca_lpd_detect",
            "drivers/power/supply/mca/mca_platform/mca_platform_buckchg_class",
            "drivers/power/supply/mca/mca_platform/mca_platform_cp_class",
            "drivers/power/supply/mca/mca_platform/mca_platform_wireless_class",
            "drivers/power/supply/mca/mca_protocol/mca_protocol_class",
            "drivers/power/supply/mca/mca_shared_memory/charger_partition",
            "drivers/power/supply/mca/mca_strategy/mca_strategy_class",
            "drivers/power/supply/mca/mca_strategy/strategy_fg/mca_strategy_fg_class",
            "drivers/power/supply/mca/mca_strategy/strategy_wireless/mca_wireless_revchg",
        ],
    )
