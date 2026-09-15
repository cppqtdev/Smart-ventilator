# Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
# SPDX-License-Identifier: LicenseRef-Proprietary
# Smart Ventilator ICU Application

# Collect resource files
file(GLOB SV_ICON_FILES  ${CMAKE_CURRENT_SOURCE_DIR}/icons/*)
file(GLOB SV_AUDIO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/audio/*)
file(GLOB SV_FONT_FILES  ${CMAKE_CURRENT_SOURCE_DIR}/fonts/*)

set(SV_RESOURCE_FILES
    ${SV_ICON_FILES}
    ${SV_AUDIO_FILES}
    ${SV_FONT_FILES}
)

if(SV_RESOURCE_FILES)
    qt_add_resources(sv_resources "sv_assets"
        PREFIX "/assets"
        FILES ${SV_RESOURCE_FILES}
    )
else()
    # Empty placeholder target until resource files are added
    add_library(sv_resources INTERFACE)
endif()
