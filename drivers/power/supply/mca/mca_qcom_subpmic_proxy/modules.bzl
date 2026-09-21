def register_modules(registry):
    registry.register(
        name = "drivers/power/supply/mca/mca_qcom_subpmic_proxy/mca_qcom_subpmic_proxy",
        out = "mca_qcom_subpmic_proxy.ko",
        config = "CONFIG_MCA_QCOM_SUBPMIC",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_qcom_subpmic_proxy/mca_qcom_subpmic_proxy.c",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log",
            "drivers/power/supply/mca/mca_common/mca_event",
            "drivers/power/supply/mca/mca_common/mca_hwid",
            "drivers/power/supply/mca/mca_common/mca_parse_dts",
            "drivers/power/supply/mca/mca_common/mca_adsp_glink",
            "drivers/power/supply/mca/mca_platform/mca_platform_bc12_class",
            "drivers/power/supply/mca/mca_platform/mca_platform_buckchg_class",
            "drivers/power/supply/mca/mca_platform/mca_platform_cp_class",
            "drivers/power/supply/mca/mca_protocol/mca_protocol_class",
            "drivers/power/supply/mca/mca_protocol/protocol_qc/mca_protocol_qc_class",
            "drivers/power/supply/mca/mca_protocol/protocol_pd/mca_protocol_pd_class",
            "drivers/power/supply/mca/mca_strategy/mca_strategy_class",
        ],
    )
