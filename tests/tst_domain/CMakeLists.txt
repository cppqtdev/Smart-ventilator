# Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
# SPDX-License-Identifier: LicenseRef-Proprietary
# Smart Ventilator ICU Application

set(DOMAIN_TEST_SOURCES
    tst_patient.cpp
    tst_ventilation_mode.cpp
    tst_parameter_catalog.cpp
)

foreach(test_source ${DOMAIN_TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} PRIVATE
        sv_domain
        sv_common
        Qt6::Test
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
    sv_set_warnings(${test_name})
endforeach()
