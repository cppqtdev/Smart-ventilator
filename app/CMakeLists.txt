# Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
# SPDX-License-Identifier: LicenseRef-Proprietary
# Smart Ventilator ICU Application

qt_add_executable(SmartVentilator
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Application.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Application.cpp
)

# Project root needed for legacy controller includes during transition
target_include_directories(SmartVentilator PRIVATE
    ${PROJECT_SOURCE_DIR}
)

target_link_libraries(SmartVentilator PRIVATE
    sv_common
    sv_legacy_core
    sv_legacy_controllers
    sv_domain
    sv_infrastructure
    sv_services
    sv_models
    sv_backend
    sv_render
    sv_presentation
    sv_transport
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Qml
    Qt6::Multimedia
    Qt6::Sql
)

# Link resources if available
if(TARGET sv_resources)
    target_link_libraries(SmartVentilator PRIVATE sv_resources)
endif()

target_link_libraries(SmartVentilator PRIVATE
    sv_ui_theme
    sv_ui_controls
    sv_ui_components
    sv_ui_dialogs
    sv_ui_screens
    sv_ui_assets
    sv_ui_shell
)

sv_set_warnings(SmartVentilator)

install(TARGETS SmartVentilator
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
