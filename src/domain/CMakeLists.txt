# Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
# SPDX-License-Identifier: LicenseRef-Proprietary
# Smart Ventilator ICU Application

file(GLOB_RECURSE SV_DOMAIN_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
)

add_library(sv_domain STATIC ${SV_DOMAIN_SOURCES})

target_include_directories(sv_domain PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(sv_domain PUBLIC
    sv_common
)

sv_set_warnings(sv_domain)
