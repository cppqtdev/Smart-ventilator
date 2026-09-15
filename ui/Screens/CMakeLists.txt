# Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
# SPDX-License-Identifier: LicenseRef-Proprietary
# Smart Ventilator ICU Application

# UI module: top-level device screens.

file(GLOB SV_UI_SCREEN_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/Home/*.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/Home/qmldir
    ${CMAKE_CURRENT_SOURCE_DIR}/Panes/*.qml
)

sv_add_ui_resource(sv_ui_screens sv_ui_screens ${SV_UI_SCREEN_FILES})
