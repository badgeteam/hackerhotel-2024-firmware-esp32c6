idf_component_register(
  REQUIRES driver riscv
  SRCS "ch32_ll.c" "ch32.c"
  "generated/ch32_writemem.S.c"
  "generated/ch32_readmem.S.c"
  INCLUDE_DIRS include
)
