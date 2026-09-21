def register_modules(registry):
    registry.register(
        name = "drivers/power/supply/mca/mca_business/business_misc/mca_business_misc_comp",
        out = "mca_business_misc_comp.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_business/business_misc/inc/mca_business_misc.h",
            "drivers/power/supply/mca/mca_business/business_misc/mca_business_misc.c",
            "drivers/power/supply/mca/mca_business/business_misc/mca_business_votable.c",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_sysfs",
            "drivers/power/supply/mca/mca_common/mca_event",
            "drivers/power/supply/mca/mca_common/mca_log",
            "drivers/power/supply/mca/mca_common",
        ],
    )
