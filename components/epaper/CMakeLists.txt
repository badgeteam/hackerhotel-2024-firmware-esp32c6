idf_component_register(
  REQUIRES driver
           debug-tools
           nvs_flash
  SRCS "epaper.c"
       "epaper_lut.c"
  INCLUDE_DIRS include
)
