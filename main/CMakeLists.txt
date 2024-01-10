idf_component_register(
    SRCS "main.c"
         "board.c"
    INCLUDE_DIRS "."
    EMBED_FILES ${project_dir}/resources/renze.png
                ${project_dir}/resources/ch32_firmware.bin
)
