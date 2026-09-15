# Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
# SPDX-License-Identifier: LicenseRef-Proprietary
# Smart Ventilator ICU Application

# UI module: reusable touch controls and clinical chart controls.

file(GLOB SV_UI_CONTROL_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.qml
)

sv_add_ui_resource(sv_ui_controls sv_ui_controls ${SV_UI_CONTROL_FILES})
