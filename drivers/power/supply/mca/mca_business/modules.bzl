load(":drivers/power/supply/mca/mca_business/business_battery/modules.bzl", register_business_battery = "register_modules")

def register_modules(registry):
    register_business_battery(registry)
