idf_component_register(
        SRCS "badge_comms.c" "ieee802154.c"
        INCLUDE_DIRS "include"
        PRIV_INCLUDE_DIRS "priv_include"
        REQUIRES
        "nvs_flash"
        "ieee802154"
)
