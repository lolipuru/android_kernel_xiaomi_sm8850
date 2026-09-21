def register_modules(registry):
    registry.register(
        name = "drivers/power/supply/mca/mca_shared_memory/charger_partition",
        out = "charger_partition.ko",
        config = "CONFIG_CHARGER_PARTITION",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_shared_memory/inc/charger_partition.h",
            "drivers/power/supply/mca/mca_shared_memory/charger_partition.c",
        ],
        hdrs = [
            "drivers/power/supply/mca/include/mca/shared_memory/charger_partition_class.h",
        ],
        includes = [
            "drivers/power/supply/mca/include",
            "drivers/scsi",
            "drivers/misc/hwid",
        ],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_sysfs",
            "drivers/power/supply/mca/mca_common/mca_event",
            "drivers/power/supply/mca/mca_common/mca_log",
            "drivers/power/supply/mca/mca_common/mca_hwid",
            "drivers/power/supply/mca/mca_strategy/mca_strategy_class",
            "drivers/misc/hwid",
        ],
    )
