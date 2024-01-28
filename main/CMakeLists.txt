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
    EMBED_FILES ${project_dir}/resources/bodgeteam.png
                ${project_dir}/resources/textedit.png
                ${project_dir}/resources/capslock.png
                ${project_dir}/resources/special.png
                ${project_dir}/resources/page1.png
                ${project_dir}/resources/page2.png
                ${project_dir}/resources/page3.png
                ${project_dir}/resources/mascots.png
                ${project_dir}/resources/shades.png
                ${project_dir}/resources/homescreen.png
                ${project_dir}/resources/settings.png
                ${project_dir}/resources/border1.png
                ${project_dir}/resources/border2.png
                ${project_dir}/resources/caronl.png
                ${project_dir}/resources/caronv.png
                ${project_dir}/resources/carond.png
)
