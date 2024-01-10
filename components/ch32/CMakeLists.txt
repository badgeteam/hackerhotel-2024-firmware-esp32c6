idf_component_register(
  REQUIRES driver
  SRCS "ch32_ll.c" "ch32.c" "ch32_programmer.c"
  "generated/ch32_writemem.S.c"
  "generated/ch32_readmem.S.c"
  INCLUDE_DIRS include
)
