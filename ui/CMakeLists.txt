function(sv_add_ui_resource target resource_name)
    qt_add_library(${target} STATIC)
    qt_add_resources(${target} "${resource_name}"
        PREFIX "/"
        BASE ${PROJECT_SOURCE_DIR}
        FILES ${ARGN}
    )
endfunction()

add_subdirectory(Theme)
add_subdirectory(Controls)
add_subdirectory(Components)
add_subdirectory(Dialogs)
add_subdirectory(Screens)
add_subdirectory(Assets)
add_subdirectory(Shell)
