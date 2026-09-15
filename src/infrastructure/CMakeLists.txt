# Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
# SPDX-License-Identifier: LicenseRef-Proprietary
# Smart Ventilator ICU Application

set(SV_INFRA_SOURCES
    src/SimulatorAdapter.cpp
)

set(SV_INFRA_HEADERS
    include/sv/infrastructure/IVentilatorHardware.h
    include/sv/infrastructure/SimulatorAdapter.h
)

add_library(sv_infrastructure STATIC ${SV_INFRA_SOURCES} ${SV_INFRA_HEADERS})

target_include_directories(sv_infrastructure PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(sv_infrastructure PUBLIC
    sv_domain
    sv_common
)

sv_set_warnings(sv_infrastructure)
