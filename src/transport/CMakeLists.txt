# Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
# SPDX-License-Identifier: LicenseRef-Proprietary
# Smart Ventilator ICU Application

# SerialBus is an optional Qt module and is not installed with every kit.
# Without it the CAN link is compiled out and the simulator is the only
# telemetry source, so a desktop kit that lacks it still builds and runs.
find_package(Qt6 QUIET COMPONENTS SerialBus)

set(SV_TRANSPORT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CanSignal.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SimulatedTelemetrySource.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TelemetryFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TelemetryBridge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/UdpTelemetrySource.cpp
)

# The headers have to be sources of the target or the meta-object compiler
# never sees them: every class here carries Q_OBJECT, and without their
# generated code the library has no static meta-objects and no vtables, which
# is a link failure and not a compile one. Every other module globs its
# headers in for the same reason. ITelemetrySource.h has no .cpp of its own,
# so nothing would have reached it either way.
set(SV_TRANSPORT_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/sv/transport/CanSignal.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/sv/transport/ITelemetrySource.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/sv/transport/SimulatedTelemetrySource.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/sv/transport/TelemetryBridge.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/sv/transport/TelemetryFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/sv/transport/UdpTelemetrySource.h
)

if(Qt6SerialBus_FOUND)
    list(APPEND SV_TRANSPORT_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/CanTelemetrySource.cpp)
    list(APPEND SV_TRANSPORT_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/include/sv/transport/CanTelemetrySource.h)
endif()

qt_add_library(sv_transport STATIC ${SV_TRANSPORT_SOURCES} ${SV_TRANSPORT_HEADERS})

qt_add_resources(sv_transport "sv_transport_data"
    PREFIX "/data"
    BASE ${CMAKE_CURRENT_SOURCE_DIR}/data
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/ventilator.dbc
)

target_include_directories(sv_transport PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(sv_transport PUBLIC Qt6::Core Qt6::Network sv_common sv_legacy_controllers)

if(Qt6SerialBus_FOUND)
    target_link_libraries(sv_transport PUBLIC Qt6::SerialBus)
    target_compile_definitions(sv_transport PUBLIC SV_HAS_CANBUS=1)
    message(STATUS "sv_transport: CAN link enabled (Qt6::SerialBus found)")
else()
    target_compile_definitions(sv_transport PUBLIC SV_HAS_CANBUS=0)
    message(STATUS "sv_transport: Qt6::SerialBus not found, simulator only")
endif()

sv_set_warnings(sv_transport)
