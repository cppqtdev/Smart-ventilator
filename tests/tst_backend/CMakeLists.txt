# Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
# SPDX-License-Identifier: LicenseRef-Proprietary
# Smart Ventilator ICU Application

file(GLOB_RECURSE TST_BACKEND_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

if(TST_BACKEND_SOURCES)
    add_executable(tst_backend ${TST_BACKEND_SOURCES})

    target_link_libraries(tst_backend PRIVATE
        sv_backend
        sv_services
        sv_domain
        sv_infrastructure
        sv_models
        sv_common
        Qt6::Test
        Qt6::Quick
        Qt6::Qml
    )

    add_test(NAME tst_backend COMMAND tst_backend)
    sv_set_warnings(tst_backend)
endif()
