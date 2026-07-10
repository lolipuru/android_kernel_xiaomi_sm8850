load(":drivers/power/supply/mca/mca_common/modules.bzl", register_mca_common = "register_modules")

def register_modules(registry):
    register_mca_common(registry)
