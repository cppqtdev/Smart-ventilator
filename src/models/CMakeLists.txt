# Copyright (c) 2024-2026 TechCoderHub LLP. All rights reserved.
# SPDX-License-Identifier: LicenseRef-Proprietary
# Smart Ventilator ICU Application

file(GLOB_RECURSE SV_MODELS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
)

# The module is C++ only and currently empty. A STATIC library with no
# sources fails the configure step outright, so it is declared as an
# interface target until the first model class lands here.
if(SV_MODELS_SOURCES)
    add_library(sv_models STATIC ${SV_MODELS_SOURCES})
else()
    add_library(sv_models INTERFACE)
endif()

if(SV_MODELS_SOURCES)
    set(SV_MODELS_SCOPE PUBLIC)
else()
    set(SV_MODELS_SCOPE INTERFACE)
endif()

target_include_directories(sv_models ${SV_MODELS_SCOPE}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(sv_models ${SV_MODELS_SCOPE}
    sv_domain
    sv_services
    sv_common
    Qt6::Core
    Qt6::Qml
)

# QML module registration (uncomment when QML-exposed model classes are ready)
# qt_add_qml_module(sv_models
#     URI sv.models
#     VERSION 1.0
#     # QML_FILES (none - C++ only module)
#     # SOURCES are already part of the STATIC library above
# )

if(SV_MODELS_SOURCES)
    sv_set_warnings(sv_models)
endif()
