# Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
# SPDX-License-Identifier: LicenseRef-Proprietary
# Smart Ventilator ICU Application

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(OECORE_TARGET_SYSROOT $ENV{OECORE_TARGET_SYSROOT})

if(NOT OECORE_TARGET_SYSROOT)
    message(FATAL_ERROR "OECORE_TARGET_SYSROOT environment variable is not set. "
        "Source the Yocto/OE SDK environment script before configuring.")
endif()

set(CMAKE_SYSROOT ${OECORE_TARGET_SYSROOT})

set(CMAKE_C_COMPILER   aarch64-poky-linux-gcc)
set(CMAKE_CXX_COMPILER aarch64-poky-linux-g++)

set(CMAKE_FIND_ROOT_PATH ${OECORE_TARGET_SYSROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
