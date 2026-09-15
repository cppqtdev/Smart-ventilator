# Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
# SPDX-License-Identifier: LicenseRef-Proprietary
# Smart Ventilator ICU Application

# UI module: image, icon, and audio assets used by QML.

file(GLOB_RECURSE SV_UI_ASSET_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.png
    ${CMAKE_CURRENT_SOURCE_DIR}/*.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/*.wav
)

sv_add_ui_resource(sv_ui_assets sv_ui_assets ${SV_UI_ASSET_FILES})
