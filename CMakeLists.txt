# Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
# SPDX-License-Identifier: LicenseRef-Proprietary
# Smart Ventilator ICU Application

cmake_minimum_required(VERSION 3.22)

project(SmartVentilator
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Build options
option(SV_BUILD_TESTS "Build unit tests" OFF)
option(SV_BUILD_APP   "Build the application executable" ON)

# CMake modules
include(cmake/CompilerWarnings.cmake)
include(cmake/QtSetup.cmake)
include(cmake/CodeCoverage.cmake)

# Libraries (order matters for dependencies)
add_subdirectory(src/common)
add_subdirectory(src/core)
add_subdirectory(src/controllers)
add_subdirectory(src/domain)
add_subdirectory(src/infrastructure)
add_subdirectory(src/services)
add_subdirectory(src/models)
add_subdirectory(src/render)
add_subdirectory(src/transport)
add_subdirectory(src/presentation)

# Resources
add_subdirectory(resources)

# UI / QML modules
add_subdirectory(ui)

# Application
if(SV_BUILD_APP)
    add_subdirectory(app)
endif()

# Tests
if(SV_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
