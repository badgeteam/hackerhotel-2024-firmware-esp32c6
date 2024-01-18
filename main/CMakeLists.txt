FILE(GLOB task_srcs tasks/*.c)

idf_component_register(
    SRCS "main.c"
         "application.c"
         "application_settings.c"
         ${task_srcs}
    INCLUDE_DIRS "." "tasks"
    EMBED_FILES ${project_dir}/resources/renze.png
)
