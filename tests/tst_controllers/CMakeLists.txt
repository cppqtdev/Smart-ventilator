# Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
# SPDX-License-Identifier: LicenseRef-Proprietary
# Smart Ventilator ICU Application

set(CONTROLLER_TEST_SOURCES
    tst_ventilator_start.cpp
    tst_monitoring_presenter.cpp
)

foreach(test_source ${CONTROLLER_TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} PRIVATE
        sv_legacy_controllers
        sv_presentation
        sv_domain
        sv_common
        Qt6::Test
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
    sv_set_warnings(${test_name})
endforeach()
