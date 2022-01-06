def register_modules(registry):
    registry.register(
        name = "drivers/misc/hwid",
        out = "hwid.ko",
        config = "CONFIG_MI_HARDWARE_ID",
        srcs = [
            # do not sort
            "drivers/misc/hwid/hwid.c",
            "drivers/misc/hwid/hwid.h",
        ],
    )
