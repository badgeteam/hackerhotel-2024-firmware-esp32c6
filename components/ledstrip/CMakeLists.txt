idf_component_register(
    SRCS "ledstrip.c" "led_strip_encoder.c"
    INCLUDE_DIRS "." "include"
    REQUIRES
        "driver"
)
