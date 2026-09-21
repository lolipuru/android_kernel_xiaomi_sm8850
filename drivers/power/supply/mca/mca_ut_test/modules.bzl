def register_modules(registry):
    registry.register(
        name = "drivers/power/supply/mca/mca_ut_test/mca_ut_test",
        out = "mca_ut_test.ko",
        config = "CONFIG_MCA_UT_TEST",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_ut_test/mca_ut_test.c",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log",
            "drivers/power/supply/mca/mca_common/mca_hwid",
            "drivers/power/supply/mca/mca_common",
            "drivers/power/supply/mca/mca_strategy/mca_strategy_class",
            "drivers/power/supply/mca/mca_strategy/strategy_fg/mca_strategy_fg_class",
            "drivers/power/supply/mca/mca_platform/mca_platform_fg_ic_ops",
            "drivers/power/supply/mca/mca_platform/mca_platform_wireless_class",
        ],
    )
