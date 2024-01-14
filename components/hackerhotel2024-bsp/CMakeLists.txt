idf_component_register(
    SRCS "bsp.c"
    INCLUDE_DIRS "." "include"
    EMBED_FILES "resources/ch32_firmware.bin"
    REQUIRES
        "riscv"
        "sdmmc"
        "fatfs"
        "nvs_flash"
        "bus-i2c"
        "epaper"
        "ch32"
        "epaper"
        "pax-gfx"
)
