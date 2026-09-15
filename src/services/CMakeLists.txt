# Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
# SPDX-License-Identifier: LicenseRef-Proprietary
# Smart Ventilator ICU Application

set(SV_SERVICES_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SettingsValidator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AlarmEvaluator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/WaveformEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/BreathSimulator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ClinicalMetricsCalculator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CalibrationService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DiagnosticsService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/RespiratoryMechanics.cpp
)

set(SV_SERVICES_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/sv/services/SettingsValidator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/sv/services/AlarmEvaluator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/sv/services/WaveformEngine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/sv/services/BreathSimulator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/sv/services/ClinicalMetricsCalculator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/sv/services/CalibrationService.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/sv/services/DiagnosticsService.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/sv/services/RespiratoryMechanics.h
)

add_library(sv_services STATIC ${SV_SERVICES_SOURCES} ${SV_SERVICES_HEADERS})

target_include_directories(sv_services PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(sv_services PUBLIC
    sv_domain
    sv_infrastructure
    sv_common
)

sv_set_warnings(sv_services)
