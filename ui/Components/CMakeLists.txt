# Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
# SPDX-License-Identifier: LicenseRef-Proprietary
# Smart Ventilator ICU Application

# UI module: composed cards, banners, status indicators, and navigation.

file(GLOB SV_UI_COMPONENT_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.qml
)

sv_add_ui_resource(sv_ui_components sv_ui_components ${SV_UI_COMPONENT_FILES})
