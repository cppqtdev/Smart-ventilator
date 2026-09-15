# Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
# SPDX-License-Identifier: LicenseRef-Proprietary
# Smart Ventilator ICU Application

qt_add_library(sv_backend STATIC
    include/sv/backend/VentilatorFacade.h
    include/sv/backend/BatteryFacade.h
    include/sv/backend/DiagnosticsFacade.h
    src/VentilatorFacade.cpp
    src/BatteryFacade.cpp
    src/DiagnosticsFacade.cpp
)

target_include_directories(sv_backend PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Legacy controllers needed during transition (VentilatorFacade uses AlarmController)
target_include_directories(sv_backend PRIVATE
    ${PROJECT_SOURCE_DIR}
)

target_link_libraries(sv_backend PUBLIC
    sv_domain
    sv_services
    sv_infrastructure
    sv_models
    sv_common
    sv_legacy_controllers
    Qt6::Core
    Qt6::Qml
    Qt6::Quick
    Qt6::Multimedia
    Qt6::Sql
)

qt_add_qml_module(sv_backend
    URI sv.backend
    VERSION 1.0
    SOURCES
        include/sv/backend/VentilatorFacade.h
        include/sv/backend/BatteryFacade.h
        include/sv/backend/DiagnosticsFacade.h
)

sv_set_warnings(sv_backend)
