def register_modules(registry):
    registry.register(
        name = "drivers/power/supply/mca/mca_qcom_sysfs/mca_qcom_sysfs",
        out = "mca_qcom_sysfs.ko",
        config = "CONFIG_MCA_QCOM_SYSFS",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_qcom_sysfs/mca_qcom_sysfs.c",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log",
            "drivers/power/supply/mca/mca_protocol/mca_protocol_class",
            "drivers/power/supply/mca/mca_protocol/protocol_pd/mca_protocol_pd_class",
            "drivers/power/supply/mca/mca_strategy/mca_strategy_class",
            "drivers/power/supply/mca/mca_strategy/strategy_fg/mca_strategy_fg_class",
            "drivers/power/supply/mca/mca_strategy/strategy_wireless/mca_basic_wireless",
            "drivers/power/supply/mca/mca_platform/mca_platform_buckchg_class",
            "drivers/power/supply/mca/mca_platform/mca_platform_cp_class",
            "drivers/power/supply/mca/mca_platform/mca_platform_wireless_class",
            "drivers/power/supply/mca/mca_strategy/strategy_charger/mca_charger_thermal",
            "drivers/power/supply/mca/mca_strategy/strategy_wireless/mca_wireless_revchg",
        ],
    )
