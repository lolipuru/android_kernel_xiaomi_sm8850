def register_modules(registry):
    registry.register(
        name = "drivers/power/supply/mca/mca_protocol/mca_protocol_class",
        out = "mca_protocol_class.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_protocol/mca_protocol_class.c",
        ],
        hdrs = [
            "drivers/power/supply/mca/include/mca/protocol/protocol_class.h",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_protocol/protocol_pd/mca_protocol_pd_class",
        out = "mca_protocol_pd_class.ko",
        config = "CONFIG_MCA_PROTOCOL_PD",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_protocol/protocol_pd/mca_protocol_pd_class.c",
        ],
        hdrs = [
            "drivers/power/supply/mca/include/mca/protocol/protocol_pd_class.h",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log",
            "drivers/power/supply/mca/mca_common/mca_event",
            "drivers/power/supply/mca/mca_common/mca_parse_dts",
            "drivers/power/supply/mca/mca_common/mca_sysfs",
            "drivers/power/supply/mca/mca_protocol/mca_protocol_class",
            "drivers/power/supply/mca/mca_shared_memory/charger_partition",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_protocol/protocol_pd/qcom_tcpc/qcom_adsp_pd_protocol",
        out = "qcom_adsp_pd_protocol.ko",
        config = "CONFIG_MCA_QCOM_ADSP",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_protocol/protocol_pd/qcom_tcpc/qcom_adsp_pd_protocol.c",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log",
            "drivers/power/supply/mca/mca_common/mca_event",
            "drivers/power/supply/mca/mca_common/mca_adsp_glink",
            "drivers/power/supply/mca/mca_protocol/mca_protocol_class",
            "drivers/power/supply/mca/mca_protocol/protocol_pd/mca_protocol_pd_class",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_protocol/protocol_qc/mca_protocol_qc_class",
        out = "mca_protocol_qc_class.ko",
        config = "CONFIG_MCA_PROTOCOL_QC",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_protocol/protocol_qc/mca_protocol_qc_class.c",
        ],
        hdrs = [
            "drivers/power/supply/mca/include/mca/protocol/protocol_qc_class.h",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log",
            "drivers/power/supply/mca/mca_protocol/mca_protocol_class",
        ],
    )
