# Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
# SPDX-License-Identifier: LicenseRef-Proprietary
# Smart Ventilator ICU Application

# Scene-graph rendering items. Kept in its own library so the geometry
# builders can be unit-tested and profiled without dragging in the
# controllers.

file(GLOB_RECURSE SV_RENDER_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
)

add_library(sv_render STATIC ${SV_RENDER_SOURCES})

target_include_directories(sv_render PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(sv_render PUBLIC
    sv_common
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
)

sv_set_warnings(sv_render)
