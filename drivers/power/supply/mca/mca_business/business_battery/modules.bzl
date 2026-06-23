def register_modules(registry):
    registry.register(
        name = "drivers/power/supply/mca/mca_business/business_battery/mca_business_battery_comp",
        out = "mca_business_battery_comp.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_business/business_battery/inc/mca_battery_psy.h",
            "drivers/power/supply/mca/mca_business/business_battery/inc/mca_business_battery.h",
            "drivers/power/supply/mca/mca_business/business_battery/mca_battery_psy.c",
            "drivers/power/supply/mca/mca_business/business_battery/mca_business_battery.c",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_sysfs",
            "drivers/power/supply/mca/mca_common/mca_event",
            "drivers/power/supply/mca/mca_common/mca_log",
            "drivers/power/supply/mca/mca_common/mca_parse_dts",
            "drivers/power/supply/mca/mca_platform/mca_platform_buckchg_class",
        ],
    )
