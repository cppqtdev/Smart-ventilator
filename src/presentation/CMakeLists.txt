# Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
# SPDX-License-Identifier: LicenseRef-Proprietary
# Smart Ventilator ICU Application

file(GLOB_RECURSE SV_PRESENTATION_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
)

add_library(sv_presentation STATIC ${SV_PRESENTATION_SOURCES})

target_include_directories(sv_presentation PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(sv_presentation PUBLIC
    Qt6::Core
    sv_common
    sv_domain
    sv_legacy_controllers
)

sv_set_warnings(sv_presentation)
