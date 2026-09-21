def register_modules(registry):
    registry.register(
        name = "drivers/thermal/xiaomi/mi_thermal_interface",
        out = "mi_thermal_interface.ko",
        config = "CONFIG_MI_THERMAL_INTERFACE",
        srcs = [
            # do not sort
            "drivers/thermal/xiaomi/mi_thermal_interface.c",
        ],
        includes = [
            "drivers/power/supply/mca/include",
        ],
        deps = [
            # do not sort
            "drivers/soc/qcom/panel_event_notifier",
            "drivers/power/supply/mca/mca_smart_charge/mca_smart_charge",
            "drivers/power/supply/mca/mca_common/mca_event",
        ],
    )
