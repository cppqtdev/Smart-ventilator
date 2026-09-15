# Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
# SPDX-License-Identifier: LicenseRef-Proprietary
# Smart Ventilator ICU Application

# UI module: modal touch workflows for protected actions.

file(GLOB SV_UI_DIALOG_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.qml
)

sv_add_ui_resource(sv_ui_dialogs sv_ui_dialogs ${SV_UI_DIALOG_FILES})
