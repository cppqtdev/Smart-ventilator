# Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
# SPDX-License-Identifier: LicenseRef-Proprietary
# Smart Ventilator ICU Application

file(GLOB_RECURSE TST_MODELS_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

if(TST_MODELS_SOURCES)
    add_executable(tst_models ${TST_MODELS_SOURCES})

    target_link_libraries(tst_models PRIVATE
        sv_models
        sv_domain
        sv_services
        sv_common
        Qt6::Test
        Qt6::Qml
    )

    add_test(NAME tst_models COMMAND tst_models)
    sv_set_warnings(tst_models)
endif()
