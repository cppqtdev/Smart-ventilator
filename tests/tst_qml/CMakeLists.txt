# Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
# SPDX-License-Identifier: LicenseRef-Proprietary
# Smart Ventilator ICU Application

find_package(Qt6 REQUIRED COMPONENTS QuickTest Qml Quick)

add_executable(tst_qml tst_qml_main.cpp)

target_link_libraries(tst_qml PRIVATE
    Qt6::QuickTest
    Qt6::Qml
    Qt6::Quick
    sv_ui_theme
    sv_ui_controls
    sv_ui_components
    sv_ui_screens
    sv_ui_assets
    sv_render
)

target_compile_definitions(tst_qml PRIVATE
    QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

add_test(NAME tst_qml COMMAND tst_qml -input ${CMAKE_CURRENT_SOURCE_DIR})
sv_set_warnings(tst_qml)
