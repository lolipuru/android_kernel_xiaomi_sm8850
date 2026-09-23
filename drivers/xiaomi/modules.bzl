load(":drivers/xiaomi/boottime/modules.bzl", register_boottime = "register_modules")
load(":drivers/xiaomi/bootmonitor/modules.bzl", register_bootmonitor = "register_modules")
load(":drivers/xiaomi/mtdoops/modules.bzl", register_mtdoops = "register_modules")
load(":drivers/xiaomi/swinfo/modules.bzl", register_swinfo = "register_modules")

def register_modules(registry):
    register_boottime(registry)
    register_bootmonitor(registry)
    register_mtdoops(registry)
    register_swinfo(registry)

