def register_modules(registry):
    registry.register(
        name = "drivers/power/supply/mca/mca_common/mca_sysfs",
        out = "mca_sysfs.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_sysfs.c",
        ],
        hdrs = [
            "drivers/power/supply/mca/include/mca/common/mca_sysfs.h",
        ],
        includes = ["drivers/power/supply/mca/include"],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_common/mca_event",
        out = "mca_event.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_event.c",
        ],
        hdrs = [
            "drivers/power/supply/mca/include/mca/common/mca_event.h",
        ],
        includes = ["drivers/power/supply/mca/include"],
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
        hdrs = [
            "drivers/power/supply/mca/include/mca/common/mca_log.h",
        ],
        includes = ["drivers/power/supply/mca/include"],
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
        hdrs = [
            "drivers/power/supply/mca/include/mca/common/mca_parse_dts.h",
        ],
        includes = ["drivers/power/supply/mca/include"],
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
        hdrs = [
            "drivers/power/supply/mca/include/mca/common/mca_workqueue.h",
        ],
        includes = ["drivers/power/supply/mca/include"],
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
        hdrs = [
            "drivers/power/supply/mca/include/mca/common/mca_voter.h",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_common/mca_charge_mievent",
        out = "mca_charge_mievent.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_charge_mievent.c",
        ],
        hdrs = [
            "drivers/power/supply/mca/include/mca/common/mca_charge_mievent.h",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log",
            "drivers/misc/miev",
            "drivers/power/supply/mca/mca_common/mca_event",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_common/mca_hwid",
        out = "mca_hwid.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_hwid.c",
        ],
        hdrs = [
            "drivers/power/supply/mca/include/mca/common/mca_hwid.h",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log",
            "drivers/misc/hwid",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_common/mca_adsp_glink",
        out = "mca_adsp_glink.ko",
        config = "CONFIG_MCA_QCOM_ADSP",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_adsp_glink.c",
        ],
        hdrs = [
            "drivers/power/supply/mca/include/mca/common/mca_adsp_glink.h",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log",
            "drivers/soc/qcom/qti_pmic_glink",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_common/mca_charge_interface",
        out = "mca_charge_interface.ko",
        config = "CONFIG_XM_POWER_SUPPLY",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_charge_interface.c",
        ],
        hdrs = [
            "drivers/power/supply/mca/include/mca/common/mca_charge_interface.h",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log",
            "drivers/power/supply/mca/mca_common/mca_event",
            "drivers/power/supply/mca/mca_common/mca_sysfs",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_common/mca_qcom_panel",
        out = "mca_qcom_panel.ko",
        config = "CONFIG_MCA_QCOM_PANEL",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_qcom_panel.c",
        ],
        hdrs = [
            "drivers/power/supply/mca/include/mca/common/mca_panel.h",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log",
            "drivers/soc/qcom/panel_event_notifier",
            "drivers/power/supply/mca/mca_common/mca_event",
        ],
    )

    registry.register(
        name = "drivers/power/supply/mca/mca_common/mca_qcom_smem",
        out = "mca_qcom_smem.ko",
        config = "CONFIG_MCA_QCOM_SMEM",
        srcs = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_qcom_smem.c",
        ],
        hdrs = [
            "drivers/power/supply/mca/include/mca/common/mca_smem.h",
        ],
        includes = ["drivers/power/supply/mca/include"],
        deps = [
            # do not sort
            "drivers/power/supply/mca/mca_common/mca_log",
            "drivers/soc/qcom/smem",
        ],
    )
