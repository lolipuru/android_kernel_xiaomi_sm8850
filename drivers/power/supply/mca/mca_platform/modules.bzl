def register_modules(registry):
    registry.register(
        name = "drivers/power/supply/mca/mca_platform/mca_platform_base",
        out = "mca_platform_base.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_platform/mca_platform_base.c",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_platform/mca_platform_buckchg_class",
        out = "mca_platform_buckchg_class.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_platform/mca_platform_buckchg_class.c",
        ],
        hdrs = [
            "drivers/power/supply/mca/include/mca/platform/platform_buckchg_class.h",
        ],
        includes = ["drivers/power/supply/mca/include"],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_platform/mca_platform_fg_ic_ops",
        out = "mca_platform_fg_ic_ops.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_platform/mca_platform_fg_ic_ops.c",
        ],
        hdrs = [
            "drivers/power/supply/mca/include/mca/platform/platform_fg_ic_ops.h",
        ],
        includes = ["drivers/power/supply/mca/include"],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_platform/mca_platform_bc12_class",
        out = "mca_platform_bc12_class.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_platform/mca_platform_bc12_class.c",
        ],
        hdrs = [
            "drivers/power/supply/mca/include/mca/platform/platform_bc12_class.h",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log",
            "drivers/power/supply/mca/mca_protocol/mca_protocol_class",
            "drivers/power/supply/mca/mca_protocol/protocol_pd/mca_protocol_pd_class",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_platform/mca_platform_cp_class",
        out = "mca_platform_cp_class.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_platform/mca_platform_cp_class.c",
        ],
        hdrs = [
            "drivers/power/supply/mca/include/mca/platform/platform_cp_class.h",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log",
            "drivers/power/supply/mca/mca_common/mca_parse_dts",
            "drivers/power/supply/mca/mca_common/mca_sysfs",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_platform/mca_platform_loadsw_class",
        out = "mca_platform_loadsw_class.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_platform/mca_platform_loadsw_class.c",
        ],
        hdrs = [
            "drivers/power/supply/mca/include/mca/platform/platform_loadsw_class.h",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log",
            "drivers/power/supply/mca/mca_common/mca_parse_dts",
            "drivers/power/supply/mca/mca_common/mca_sysfs",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_platform/mca_platform_wireless_class",
        out = "mca_platform_wireless_class.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_platform/mca_platform_wireless_class.c",
        ],
        hdrs = [
            "drivers/power/supply/mca/include/mca/platform/platform_wireless_class.h",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log",
            "drivers/power/supply/mca/mca_common/mca_parse_dts",
            "drivers/power/supply/mca/mca_common/mca_sysfs",
        ],
    )
