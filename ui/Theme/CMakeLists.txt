# Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
# SPDX-License-Identifier: LicenseRef-Proprietary
# Smart Ventilator ICU Application

# UI module: theme singletons and shared visual constants.

file(GLOB SV_UI_THEME_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/qmldir
)

sv_add_ui_resource(sv_ui_theme sv_ui_theme ${SV_UI_THEME_FILES})
