load(":drivers/thermal/qcom/modules.bzl", register_thermal_qcom = "register_modules")
load(":drivers/thermal/xiaomi/modules.bzl", register_thermal_xiaomi = "register_modules")

def register_modules(registry):
    register_thermal_qcom(registry)
    register_thermal_xiaomi(registry)
