idf_component_register(
    SRCS "printer.c"
    INCLUDE_DIRS "." "include"
    REQUIRES
        "bus-i2c"
        "hackerhotel2024-bsp"
)
