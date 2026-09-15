# Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
# SPDX-License-Identifier: LicenseRef-Proprietary
# Smart Ventilator ICU Application

file(GLOB_RECURSE SV_COMMON_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
)

# LogBuffer is a QObject, so this needs moc.
qt_add_library(sv_common STATIC ${SV_COMMON_SOURCES})

target_include_directories(sv_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(sv_common PUBLIC Qt6::Core)

sv_set_warnings(sv_common)
