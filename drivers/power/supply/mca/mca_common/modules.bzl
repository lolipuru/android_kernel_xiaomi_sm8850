def register_modules(registry):
    registry.register(
        name = "drivers/power/supply/mca/mca_common/mca_sysfs",
        out = "mca_sysfs.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_sysfs.c",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_common/mca_event",
        out = "mca_event.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_event.c",
        ],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_sysfs",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_common/mca_log",
        out = "mca_log.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log.c",
        ],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_sysfs",
            "drivers/power/supply/mca/mca_common/mca_event",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_common/mca_parse_dts",
        out = "mca_parse_dts.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_parse_dts.c",
        ],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_common/mca_workqueue",
        out = "mca_workqueue.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_workqueue.c",
        ],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_common",
        out = "mca_common.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_voter.c",
        ],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log",
        ],
    )
