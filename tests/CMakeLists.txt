# Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
# SPDX-License-Identifier: LicenseRef-Proprietary
# Smart Ventilator ICU Application

find_package(Qt6 REQUIRED COMPONENTS Test)

add_subdirectory(tst_domain)
add_subdirectory(tst_services)
add_subdirectory(tst_infrastructure)
add_subdirectory(tst_backend)
add_subdirectory(tst_models)
add_subdirectory(tst_transport)
add_subdirectory(tst_controllers)
add_subdirectory(tst_common)
add_subdirectory(tst_qml)
