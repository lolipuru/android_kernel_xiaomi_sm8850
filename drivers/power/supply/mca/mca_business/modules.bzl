load(":drivers/power/supply/mca/mca_business/business_battery/modules.bzl", register_business_battery = "register_modules")
load(":drivers/power/supply/mca/mca_business/business_charger/modules.bzl", register_business_charger = "register_modules")
load(":drivers/power/supply/mca/mca_business/business_misc/modules.bzl", register_business_misc = "register_modules")

def register_modules(registry):
    register_business_battery(registry)
    register_business_charger(registry)
    register_business_misc(registry)
