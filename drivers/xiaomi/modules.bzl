load(":drivers/xiaomi/bootmonitor/modules.bzl", register_bootmonitor = "register_modules")

def register_modules(registry):
    register_bootmonitor(registry)
