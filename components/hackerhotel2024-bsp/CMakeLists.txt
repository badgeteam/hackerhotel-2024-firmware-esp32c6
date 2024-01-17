idf_component_register(
    SRCS "bsp.c"
    INCLUDE_DIRS "." "include"
    EMBED_FILES "resources/ch32_firmware.bin"
                "resources/hackerhotel.png"
    REQUIRES
        "riscv"
        "sdmmc"
        "fatfs"
        "nvs_flash"
        "esp_adc"
        "bus-i2c"
        "epaper"
        "ch32"
        "epaper"
        "pax-gfx"
        "pax-codecs"
        "ledstrip"
)
