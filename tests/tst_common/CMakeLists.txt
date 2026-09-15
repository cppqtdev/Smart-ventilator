# Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
# SPDX-License-Identifier: LicenseRef-Proprietary
# Smart Ventilator ICU Application

# Qt-free: the bit codec and the unit conversions are plain arithmetic, so
# they build and run on any toolchain, with or without Qt present.
add_executable(tst_bitcodec_standalone tst_bitcodec_standalone.cpp)
target_include_directories(tst_bitcodec_standalone PRIVATE
    ${CMAKE_SOURCE_DIR}/src/common/include)
add_test(NAME tst_bitcodec_standalone COMMAND tst_bitcodec_standalone)
sv_set_warnings(tst_bitcodec_standalone)

add_executable(tst_patient_ranges_standalone tst_patient_ranges_standalone.cpp)
add_test(NAME tst_patient_ranges_standalone COMMAND tst_patient_ranges_standalone)
sv_set_warnings(tst_patient_ranges_standalone)
