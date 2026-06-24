def register_modules(registry):
    registry.register(
        name = "drivers/misc/miev",
        out = "miev.ko",
        config = "CONFIG_MIEV",
        srcs = [
            # do not sort
            "drivers/misc/miev/midevice.c",
            "drivers/misc/miev/midevice.h",
            "drivers/misc/miev/mievent.c",
            "include/miev/mievent.h",
        ],
    )
