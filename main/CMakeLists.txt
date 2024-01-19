FILE(GLOB task_srcs tasks/*.c)
FILE(GLOB module_srcs modules/*.c)

idf_component_register(
    SRCS "main.c"
         "application.c"
         "application_settings.c"
         ${task_srcs}
         ${module_srcs}
    INCLUDE_DIRS "."
                 "tasks"
                 "modules"
    EMBED_FILES ${project_dir}/resources/renze.png
                ${project_dir}/resources/textedit.png
                ${project_dir}/resources/capslock.png
                ${project_dir}/resources/special.png
)
