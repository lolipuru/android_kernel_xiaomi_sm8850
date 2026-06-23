def register_modules(registry):
    registry.register(
        name = "drivers/power/supply/mca/mca_platform/mca_platform_buckchg_class",
        out = "mca_platform_buckchg_class.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_platform/mca_platform_buckchg_class.c",
        ],
        includes = ["drivers/power/supply/mca/include"],
    )
