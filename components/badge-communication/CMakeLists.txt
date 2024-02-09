idf_component_register(
        SRCS "badge-communication.c" "ieee802154.c"
        INCLUDE_DIRS "include"
        REQUIRES
        "nvs_flash"
        "ieee802154"
        "hackerhotel2024-bsp"
)
