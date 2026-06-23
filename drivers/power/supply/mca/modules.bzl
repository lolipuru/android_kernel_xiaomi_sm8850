load(":drivers/power/supply/mca/mca_common/modules.bzl", register_mca_common = "register_modules")
load(":drivers/power/supply/mca/mca_platform/modules.bzl", register_mca_platform = "register_modules")
load(":drivers/power/supply/mca/mca_business/modules.bzl", register_mca_business = "register_modules")

def register_modules(registry):
    register_mca_common(registry)
    register_mca_platform(registry)
    register_mca_business(registry)
