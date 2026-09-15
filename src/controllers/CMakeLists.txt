# Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
# SPDX-License-Identifier: LicenseRef-Proprietary
# Smart Ventilator ICU Application

set(SV_LEGACY_CONTROLLER_SOURCES
    AlarmAudio.cpp
    AlarmController.cpp
    ClockController.cpp
    EventController.cpp
    PatientController.cpp
    UserController.cpp
    VentilationCatalog.cpp
    VentilatorController.cpp
)

set(SV_LEGACY_CONTROLLER_HEADERS
    AlarmAudio.h
    AlarmController.h
    ClockController.h
    EventController.h
    PatientController.h
    UserController.h
    VentilationCatalog.h
    VentilatorController.h
)

qt_add_library(sv_legacy_controllers STATIC
    ${SV_LEGACY_CONTROLLER_SOURCES}
    ${SV_LEGACY_CONTROLLER_HEADERS}
)

target_include_directories(sv_legacy_controllers PUBLIC
    ${PROJECT_SOURCE_DIR}
)

target_link_libraries(sv_legacy_controllers PUBLIC
    sv_legacy_core
    sv_domain
    sv_services
    Qt6::Core
    Qt6::Multimedia
    Qt6::Sql
)

sv_set_warnings(sv_legacy_controllers)
