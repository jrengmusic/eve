#[[*****************************************************************************
                        Codegen Annotated Source of Truth
————————————————————————————————————————————————————————————————————————————————

            ░░████████████░░████████████░░████████████░░████████████
            ░░████  ░░████░░████  ░░████░░████  ░░████    ░░████
            ░░████        ░░████  ░░████░░████            ░░████
            ░░████        ░░████████████░░████████████    ░░████
            ░░████        ░░████  ░░████        ░░████    ░░████
            ░░████  ░░████░░████  ░░████░░████  ░░████    ░░████
            ░░████████████░░████  ░░████░░████████████    ░░████

————————————————————————————————————————————————————————————————————————————————
                         FOR YOUR EYES ONLY, DO NOT EDIT
******************************************************************************]]

#[[
 @file CMakeLists.txt
 @brief EVE build manifest — a self-sufficient JUCE plugin project.

 Generated from project-info.md; every value traces to one table row.
 Edit the table, run cast, then configure.
]]

# ==============================================================================
# TOOLCHAIN — language standard, platform targets, compile database
# ==============================================================================

cmake_minimum_required(VERSION 4.2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "macOS deployment target")

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "macOS architectures")
elseif(WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# ==============================================================================
# PROJECT
# ==============================================================================

project(EVE
    VERSION 0.1.0
    LANGUAGES C CXX
    DESCRIPTION "EVE Audio Plugin"
)

message(STATUS "CAST: configuring ${PROJECT_NAME} ${PROJECT_VERSION}")

# ==============================================================================
# WARNINGS — one list per compiler, chosen once, applied to every target
# ==============================================================================

set(CAST_CLANG_WARNING_FLAGS
    -Wno-float-equal
    -Wno-sign-conversion
    -Wno-implicit-float-conversion
    -Wno-switch-enum
    -Wno-shadow
    -Wno-unused-parameter
)

set(CAST_MSVC_WARNING_FLAGS
    /permissive-
    /Zc:rvalueCast
    /W4
    /FC
    /wd4244
    /wd4100
    /wd4456
    /wd4267
    /wd4996
    /wd4611
    /wd4065
    /wd4324
    /wd4505
    /wd4459
    /wd4189
    /wd4200
    /wd4702
)

if(MSVC)
    set(CAST_WARNING_FLAGS ${CAST_MSVC_WARNING_FLAGS})
else()
    set(CAST_WARNING_FLAGS ${CAST_CLANG_WARNING_FLAGS})
endif()

add_compile_options(${CAST_WARNING_FLAGS})

# ==============================================================================
# PATHS — sibling frameworks and SDKs, resolved once
# ==============================================================================

set(CAST_JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../JUCE")
set(CAST_JAM_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../jam")
set(CAST_VULKAN_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../Vulkan")

# ==============================================================================
# JUCE — patched working copy, exact version gate
# ==============================================================================

set(CAST_JUCE_VERSION 8.0.14)

find_package(Git REQUIRED)

set(CAST_JUCE_PATCHES
    "${CAST_JAM_PATH}/patch/juce-cached-image-factory-hook.patch"
    "${CAST_JAM_PATH}/patch/juce-direct2d-helpers-visibility-hook.patch"
    "${CAST_JAM_PATH}/patch/juce-image-subsection-hook.patch"
    "${CAST_JAM_PATH}/patch/juce-vulkan-engine-hook.patch"
    "${CAST_JAM_PATH}/patch/juce-paint-update-rect-hook.patch"
)

if(WIN32)
    set(CAST_JUCE_TEMP_DIR "$ENV{TEMP}")
elseif(DEFINED ENV{TMPDIR})
    set(CAST_JUCE_TEMP_DIR "$ENV{TMPDIR}")
else()
    set(CAST_JUCE_TEMP_DIR "/tmp")
endif()

set(CAST_PATCHED_JUCE_PATH "${CAST_JUCE_TEMP_DIR}/juce-patched")
set(CAST_JUCE_PATCH_STAMP_FILE "${CAST_PATCHED_JUCE_PATH}/.patch-stamp")

set(CAST_JUCE_PATCH_SET_CONCAT "")
foreach(CAST_JUCE_PATCH ${CAST_JUCE_PATCHES})
    file(READ "${CAST_JUCE_PATCH}" CAST_JUCE_PATCH_CONTENT)
    string(APPEND CAST_JUCE_PATCH_SET_CONCAT "${CAST_JUCE_PATCH_CONTENT}")
endforeach()

set(CAST_JUCE_PATCH_SET_CONCAT_FILE "${CMAKE_BINARY_DIR}/cast_juce_patch_set_concat.tmp")
file(WRITE "${CAST_JUCE_PATCH_SET_CONCAT_FILE}" "${CAST_JUCE_PATCH_SET_CONCAT}")
file(SHA256 "${CAST_JUCE_PATCH_SET_CONCAT_FILE}" CAST_JUCE_PATCH_SET_HASH)
file(REMOVE "${CAST_JUCE_PATCH_SET_CONCAT_FILE}")

set(CAST_JUCE_PATCH_STAMP_CURRENT "")
if(EXISTS "${CAST_JUCE_PATCH_STAMP_FILE}")
    file(READ "${CAST_JUCE_PATCH_STAMP_FILE}" CAST_JUCE_PATCH_STAMP_CURRENT)
    string(STRIP "${CAST_JUCE_PATCH_STAMP_CURRENT}" CAST_JUCE_PATCH_STAMP_CURRENT)
endif()

if(NOT (EXISTS "${CAST_PATCHED_JUCE_PATH}" AND CAST_JUCE_PATCH_STAMP_CURRENT STREQUAL CAST_JUCE_PATCH_SET_HASH))
    message(STATUS "CAST: patching JUCE into ${CAST_PATCHED_JUCE_PATH}")

    if(EXISTS "${CAST_PATCHED_JUCE_PATH}")
        file(REMOVE_RECURSE "${CAST_PATCHED_JUCE_PATH}")
    endif()

    file(COPY "${CAST_JUCE_PATH}/"
        DESTINATION "${CAST_PATCHED_JUCE_PATH}"
        PATTERN ".git" EXCLUDE
        PATTERN "docs" EXCLUDE
        PATTERN "DOCS.html" EXCLUDE
    )

    foreach(CAST_JUCE_PATCH ${CAST_JUCE_PATCHES})
        get_filename_component(CAST_JUCE_PATCH_NAME "${CAST_JUCE_PATCH}" NAME)

        execute_process(
            COMMAND "${GIT_EXECUTABLE}" apply --check --ignore-whitespace "${CAST_JUCE_PATCH}"
            WORKING_DIRECTORY "${CAST_PATCHED_JUCE_PATH}"
            RESULT_VARIABLE CAST_JUCE_PATCH_CHECK_RESULT
            ERROR_VARIABLE CAST_JUCE_PATCH_CHECK_ERROR
        )
        if(NOT CAST_JUCE_PATCH_CHECK_RESULT EQUAL 0)
            message(FATAL_ERROR "CAST: JUCE patch failed --check: ${CAST_JUCE_PATCH_NAME}\n${CAST_JUCE_PATCH_CHECK_ERROR}")
        endif()

        execute_process(
            COMMAND "${GIT_EXECUTABLE}" apply --ignore-whitespace "${CAST_JUCE_PATCH}"
            WORKING_DIRECTORY "${CAST_PATCHED_JUCE_PATH}"
            RESULT_VARIABLE CAST_JUCE_PATCH_RESULT
            ERROR_VARIABLE CAST_JUCE_PATCH_ERROR
        )
        if(NOT CAST_JUCE_PATCH_RESULT EQUAL 0)
            message(FATAL_ERROR "CAST: JUCE patch failed to apply: ${CAST_JUCE_PATCH_NAME}\n${CAST_JUCE_PATCH_ERROR}")
        endif()
    endforeach()

    file(WRITE "${CAST_JUCE_PATCH_STAMP_FILE}" "${CAST_JUCE_PATCH_SET_HASH}")
endif()

set(CAST_JUCE_PATH "${CAST_PATCHED_JUCE_PATH}")

add_subdirectory("${CAST_JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE")

get_directory_property(CAST_FOUND_JUCE_VERSION DIRECTORY "${CAST_JUCE_PATH}" DEFINITION JUCE_VERSION)

if(NOT CAST_FOUND_JUCE_VERSION VERSION_EQUAL CAST_JUCE_VERSION)
    message(FATAL_ERROR "CAST: JUCE ${CAST_FOUND_JUCE_VERSION} found, ${CAST_JUCE_VERSION} required — exact match")
endif()

message(STATUS "CAST: JUCE ${CAST_FOUND_JUCE_VERSION} patched at ${CAST_JUCE_PATH}")

# ==============================================================================
# MODULES — JAM framework modules
# ==============================================================================

juce_add_module("${CAST_JAM_PATH}/jam_core")
juce_add_module("${CAST_JAM_PATH}/jam_debug")
juce_add_module("${CAST_JAM_PATH}/jam_data_structures")
juce_add_module("${CAST_JAM_PATH}/jam_dsp")
juce_add_module("${CAST_JAM_PATH}/jam_gui")
juce_add_module("${CAST_JAM_PATH}/jam_graphics")
juce_add_module("${CAST_JAM_PATH}/jam_animation")
juce_add_module("${CAST_JAM_PATH}/jam_freetype")
juce_add_module("${CAST_JAM_PATH}/jam_vulkan")
juce_add_module("${CAST_JAM_PATH}/jam_style")
juce_add_module("${CAST_JAM_PATH}/jam_markdown")
juce_add_module("${CAST_JAM_PATH}/jam_web")
juce_add_module("${CAST_JAM_PATH}/jam_terminal")
juce_add_module("${CAST_JAM_PATH}/jam_plugin_bootstrap")

message(STATUS "CAST: JAM modules added from ${CAST_JAM_PATH}")

# ==============================================================================
# TARGET — the plugin declaration
# ==============================================================================

juce_add_plugin(${PROJECT_NAME}
    COMPANY_NAME "JRENG"
    COMPANY_COPYRIGHT "© 2025. JRENG. All rights reserved."
    COMPANY_WEBSITE "https://jrengmusic.com"
    COMPANY_EMAIL "info@jrengmusic.com"

    PLUGIN_MANUFACTURER_CODE JRNG
    PLUGIN_CODE EVE.
    BUNDLE_ID "com.jreng.EVE"

    FORMATS Standalone VST3 AU
    VST3_CATEGORIES Fx

    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE

    COPY_PLUGIN_AFTER_BUILD TRUE
)

juce_generate_juce_header(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
    Source/EVEProcessor.h
    Source/EVEProcessor.cpp
    Source/EVEView.h
    Source/EVEView.cpp
)

# ==============================================================================
# DEFINES — one call, platform and configuration routed by generator expression
# ==============================================================================

target_compile_definitions(${PROJECT_NAME} PUBLIC
    DONT_SET_USING_JUCE_NAMESPACE=1
    JAM_USING_OVERSAMPLING=0
    JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DONT_DECLARE_PROJECTINFO=1
    VMA_VULKAN_VERSION=1002000
    "$<$<CONFIG:Debug>:DEBUG=1>"
    "$<$<CONFIG:Release>:NDEBUG=1>"
    "$<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN=1>"
    "$<$<PLATFORM_ID:Windows>:NOMINMAX=1>"
    "$<$<PLATFORM_ID:Windows>:_WIN32_IE=0x0A00>"
    "$<$<PLATFORM_ID:Windows>:JUCE_CORE_INCLUDE_NATIVE_HEADERS=1>"
    "$<$<PLATFORM_ID:Windows>:JUCE_GRAPHICS_INCLUDE_DIRECT2D_HELPERS=1>"
    "$<$<PLATFORM_ID:Windows>:JUCE_CORE_INCLUDE_COM_SMART_PTR=1>"
)

# ==============================================================================
# INCLUDES — project, generated, and vendored header roots
# ==============================================================================

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Source"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/generated"
    "${CAST_JAM_PATH}/jam_freetype/freetype/include"
    "${CAST_JUCE_PATH}/modules/juce_graphics/fonts/harfbuzz"
)

if(APPLE)
    target_include_directories(${PROJECT_NAME} BEFORE PRIVATE "${CAST_VULKAN_PATH}/macOS/include")
elseif(WIN32)
    target_include_directories(${PROJECT_NAME} BEFORE PRIVATE "${CAST_VULKAN_PATH}/Include")
endif()

# ==============================================================================
# LINK — modules, then platform frameworks and loaders
# ==============================================================================

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_devices
        jam_core
        jam_debug
        jam_data_structures
        jam_dsp
        jam_gui
        jam_graphics
        jam_animation
        jam_freetype
        jam_vulkan
        jam_style
        jam_markdown
        jam_web
        jam_plugin_bootstrap
    PUBLIC
        juce::juce_recommended_config_flags
)

if(APPLE)
    add_library(moltenVK STATIC IMPORTED GLOBAL)
    set_target_properties(moltenVK PROPERTIES
        IMPORTED_LOCATION
            "${CAST_VULKAN_PATH}/macOS/lib/MoltenVK.xcframework/macos-arm64_x86_64/libMoltenVK.a"
        INTERFACE_INCLUDE_DIRECTORIES
            "${CAST_VULKAN_PATH}/macOS/include"
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE moltenVK "-framework IOKit" "-framework IOSurface")
elseif(WIN32)
    find_library(CAST_VULKAN_LOADER_LIB vulkan-1
        PATHS "${CAST_VULKAN_PATH}/Lib"
        NO_DEFAULT_PATH
        REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE "${CAST_VULKAN_LOADER_LIB}" dwmapi.lib user32.lib)
endif()

# ==============================================================================
# SOURCE FLAGS — per-file suppressions for vendored third-party sources
# ==============================================================================

if(MSVC)
    set_source_files_properties(
        "${CAST_JAM_PATH}/jam_freetype/jam_freetype.cpp"
        PROPERTIES COMPILE_FLAGS "/wd4005 /wd4804"
    )
else()
    set_source_files_properties(
        "${CAST_JAM_PATH}/jam_freetype/jam_freetype.cpp"
        "${CAST_JAM_PATH}/jam_freetype/jam_freetype.mm"
        PROPERTIES COMPILE_FLAGS "-Wno-macro-redefined"
    )
    set_source_files_properties(
        "${CAST_JAM_PATH}/jam_vulkan/jam_vulkan.cpp"
        "${CAST_JAM_PATH}/jam_vulkan/jam_vulkan.mm"
        PROPERTIES COMPILE_FLAGS "-Wno-nullability-completeness -Wno-nullability-extension -fno-strict-aliasing"
    )
endif()

target_compile_options(${PROJECT_NAME} PRIVATE ${CAST_WARNING_FLAGS})

# ==============================================================================
# SHADERS — glslc compiles every Vulkan shader to spv, dependency-tracked
# ==============================================================================

if(APPLE)
    set(CAST_VULKAN_GLSLC "${CAST_VULKAN_PATH}/macOS/bin/glslc")
elseif(WIN32)
    set(CAST_VULKAN_GLSLC "${CAST_VULKAN_PATH}/Bin/glslc.exe")
endif()

if(NOT EXISTS "${CAST_VULKAN_GLSLC}")
    message(FATAL_ERROR "CAST: glslc not found at ${CAST_VULKAN_GLSLC}")
endif()

set(CAST_SHADER_INCLUDE_DIR "${CAST_JAM_PATH}/resources/shaders")

file(GLOB_RECURSE CAST_SHADER_SOURCES CONFIGURE_DEPENDS
    "${CAST_JAM_PATH}/resources/shaders/*.vert"
    "${CAST_JAM_PATH}/resources/shaders/*.frag"
    "${CAST_JAM_PATH}/resources/shaders/*.comp"
)

foreach(CAST_SHADER_SOURCE ${CAST_SHADER_SOURCES})
    get_filename_component(CAST_SHADER_NAME "${CAST_SHADER_SOURCE}" NAME)
    set(CAST_SHADER_SPV "${CAST_JAM_PATH}/resources/spv/${CAST_SHADER_NAME}.spv")
    set(CAST_SHADER_DEPFILE "${CAST_SHADER_SPV}.d")
    add_custom_command(
        OUTPUT  "${CAST_SHADER_SPV}"
        COMMAND "${CAST_VULKAN_GLSLC}"
                -I "${CAST_SHADER_INCLUDE_DIR}"
                -MD -MF "${CAST_SHADER_DEPFILE}"
                "${CAST_SHADER_SOURCE}" -o "${CAST_SHADER_SPV}"
        DEPENDS "${CAST_SHADER_SOURCE}"
        DEPFILE "${CAST_SHADER_DEPFILE}"
        COMMENT "glslc: ${CAST_SHADER_NAME} -> spv"
        VERBATIM
    )
endforeach()

# ==============================================================================
# RESOURCES — embedded binary data: shaders, fonts, layout
# ==============================================================================

file(GLOB CAST_SHADER_SPV_FILES CONFIGURE_DEPENDS "${CAST_JAM_PATH}/resources/spv/*.spv")

if(CAST_SHADER_SPV_FILES)
    juce_add_binary_data(jam_vulkan_ShaderData
        NAMESPACE   "jam"
        HEADER_NAME "JamVulkanShaderData.h"
        SOURCES     ${CAST_SHADER_SPV_FILES}
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE jam_vulkan_ShaderData)
endif()

file(GLOB CAST_FONT_FILES CONFIGURE_DEPENDS "${CAST_JAM_PATH}/resources/fonts/*.ttf")

if(CAST_FONT_FILES)
    juce_add_binary_data(jam_graphics_FontData
        NAMESPACE   "jam::fonts"
        HEADER_NAME "JamFontsBinaryData.h"
        SOURCES     ${CAST_FONT_FILES}
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE jam_graphics_FontData)
endif()

file(GLOB CAST_LAYOUT_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/layout/*.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/layout/*.css"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/layout/*.xml"
    "${CAST_JAM_PATH}/resources/fonts/*.ttf"
)

juce_add_binary_data(${PROJECT_NAME}_BinaryData
    NAMESPACE "BinaryData"
    SOURCES ${CAST_LAYOUT_FILES}
            ${CAST_SHADER_SPV_FILES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_BinaryData)

# ==============================================================================
# FORMATS — per-format warning flags
# ==============================================================================

foreach(CAST_FORMAT Standalone VST3 AU)
    if(TARGET ${PROJECT_NAME}_${CAST_FORMAT})
        target_compile_options(${PROJECT_NAME}_${CAST_FORMAT} PRIVATE ${CAST_WARNING_FLAGS})
    endif()
endforeach()

# ==============================================================================
# REPORT
# ==============================================================================

message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
message(STATUS "CAST: configuration complete")
message(STATUS "CAST: generator ${CMAKE_GENERATOR}")
message(STATUS "CAST: build directory ${CMAKE_BINARY_DIR}")
message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
