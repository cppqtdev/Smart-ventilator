# Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
# SPDX-License-Identifier: LicenseRef-Proprietary
# Smart Ventilator ICU Application

set(SV_LEGACY_CORE_SOURCES
    AppSettings.cpp
    DatabaseManager.cpp
)

set(SV_LEGACY_CORE_HEADERS
    AppSettings.h
    DatabaseManager.h
)

qt_add_library(sv_legacy_core STATIC
    ${SV_LEGACY_CORE_SOURCES}
    ${SV_LEGACY_CORE_HEADERS}
)

target_include_directories(sv_legacy_core PUBLIC
    ${PROJECT_SOURCE_DIR}
)

target_compile_definitions(sv_legacy_core PRIVATE
    APP_VERSION="${PROJECT_VERSION}"
    BUILD_ID="cmake"
)

target_link_libraries(sv_legacy_core PUBLIC
    Qt6::Core
    Qt6::Sql
    sv_common
)

sv_set_warnings(sv_legacy_core)
