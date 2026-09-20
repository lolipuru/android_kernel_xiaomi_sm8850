def register_modules(registry):
    registry.register(
        name = "drivers/gpio/gpio-virtio",
        out = "gpio-virtio.ko",
        config = "CONFIG_GPIO_VIRTIO",
        srcs = [
            # do not sort
            "drivers/gpio/gpio-virtio.c",
        ],
    )
    registry.register(
        name = "drivers/gpio/gpio-pca953x",
        out = "gpio-pca953x.ko",
        config = "CONFIG_GPIO_PCA953X",
        srcs = [
            # do not sort
            "drivers/gpio/gpio-pca953x.c",
        ],
    )
    registry.register(
        name = "drivers/gpio/gpio-mi-t1",
        out = "gpio-mi-t1.ko",
        config = "CONFIG_MI_T1_CHIP",
        srcs = [
            # do not sort
            "drivers/gpio/gpio-mi-t1.c",
        ],
        deps = [
            # do not sort
            "drivers/remoteproc/rproc_qcom_common",
        ],
    )
