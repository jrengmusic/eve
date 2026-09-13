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
option(CAST_SIGN "Run the codesign/wraptool/notarize POST_BUILD chain on a Release build" ON)
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
    VERSION "0.1.0"
    LANGUAGES C CXX
    DESCRIPTION "EVE Audio Plugin"
)

set(CAST_FORMATS Standalone VST3 AU AAX)

message(STATUS "CAST: configuring ${PROJECT_NAME} ${PROJECT_VERSION}")

# ==============================================================================
# WARNINGS AND OPTIMIZATION — per-platform, per-configuration flag sets
# ==============================================================================

if(APPLE)
    set(CAST_RELEASE_COMPILE_FLAGS
-Wno-shadow;-Wno-unused-parameter;-Wno-float-equal;-Wno-sign-conversion;-Wno-switch-enum;-Wno-implicit-float-conversion;-O3;-flto=thin
    )
    set(CAST_RELEASE_LINK_FLAGS
-dead_strip
    )
    set(CAST_DEBUG_COMPILE_FLAGS
-Wno-shadow;-Wno-unused-parameter;-Wno-float-equal;-Wno-sign-conversion;-Wno-switch-enum;-Wno-implicit-float-conversion;-O0;-g
    )
elseif(WIN32)
    set(CAST_RELEASE_COMPILE_FLAGS
/wd4456;/wd4459;/wd4100;/wd4189;/wd4505;/wd4267;/wd4065;/wd4244;/wd4996;/wd4611;/wd4324;/wd4200;/wd4702;/permissive-;/Zc:rvalueCast;/W4;/FC;/O2;/GL
    )
    set(CAST_RELEASE_LINK_FLAGS
/LTCG;/OPT:REF;/OPT:ICF
    )
    set(CAST_DEBUG_COMPILE_FLAGS
/wd4456;/wd4459;/wd4100;/wd4189;/wd4505;/wd4267;/wd4065;/wd4244;/wd4996;/wd4611;/wd4324;/wd4200;/wd4702;/permissive-;/Zc:rvalueCast;/W4;/FC;/Od;/Zi
    )
endif()

# ==============================================================================
# PATHS — sibling frameworks and SDKs, resolved once
# ==============================================================================

set(CAST_JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../JUCE")
set(CAST_USER_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../jam")
set(CAST_VULKAN_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../Vulkan")

# ==============================================================================
# JUCE — patched working copy, exact version gate
# ==============================================================================

set(CAST_JUCE_VERSION 8.0.14)

find_package(Git REQUIRED)

set(CAST_JUCE_PATCHES
"${CAST_USER_MODULE_PATH}/patch/juce-cached-image-factory-hook.patch"       # External CachedComponentImage factory
"${CAST_USER_MODULE_PATH}/patch/juce-direct2d-helpers-visibility-hook.patch"# Direct2D helpers visibility gate
"${CAST_USER_MODULE_PATH}/patch/juce-image-subsection-hook.patch"           # Root image and subsection bounds hooks
"${CAST_USER_MODULE_PATH}/patch/juce-vulkan-engine-hook.patch"              # External graphics context factory for Vulkan
"${CAST_USER_MODULE_PATH}/patch/juce-paint-update-rect-hook.patch"          # OS dirty rectangle before native paint
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

juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_core")            # JAM Core
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_debug")           # Debug instruments — console, log, model monitor
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_data_structures") # ValueTree management and data model utilities — model, parameters, JSON conversion
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_dsp")             # DSP processors — filters, waveshaping, hysteresis modelling, transient control, oversampling, FIR, noise generation, spectrum analysis
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_gui")             # GUI foundation — Window, Modal, Glass
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_graphics")        # Graphics utilities, blur, shadows, colours, fonts, mesh
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_animation")       # Foundation animation classes (Animator, AnimationBase, AnimationScrollingText)
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_freetype")        # Vendored FreeType font rasterization library
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_vulkan")          # Vulkan rendering backend
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_style")           # JAM Style — LookAndFeel base + ColourScheme-backed colour registry
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_markdown")        # Clean-room native CommonMark + GFM markdown parsing and rendering
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_web")             # HTML authored-subset and CSS Syntax Level 3 subset tokenizers and parsers
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_plugin_bootstrap")# Document-driven plugin bootstrap — view construction, style management, plugin editor base, standalone shell

message(STATUS "CAST: user modules added from ${CAST_USER_MODULE_PATH}")

# ==============================================================================
# TARGET — the plugin declaration
# ==============================================================================

juce_add_plugin(${PROJECT_NAME}
    PRODUCT_NAME "EVE"
    COMPANY_NAME "JRENG"
    COMPANY_COPYRIGHT "(c) 2025 JRENG. All rights reserved."
    COMPANY_WEBSITE "https://jrengmusic.com"
    COMPANY_EMAIL "info@jrengmusic.com"

    PLUGIN_MANUFACTURER_CODE JRNG
    PLUGIN_CODE EVE.
    BUNDLE_ID "com.jreng.EVE"

    FORMATS ${CAST_FORMATS}
    VST3_CATEGORIES Fx

    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE

    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
    Source/EVEProcessor.h
    Source/EVEProcessor.cpp
    Source/EVEAudioProcessor.h
    Source/EVEView.h
    Source/EVEView.cpp
)

# ==============================================================================
# DEFINES — one call, platform and configuration routed by generator expression
# ==============================================================================

target_compile_definitions(${PROJECT_NAME} PUBLIC
    DONT_SET_USING_JUCE_NAMESPACE=1                                      # No using namespace juce in JuceHeader.h
    JAM_USING_OVERSAMPLING=0                                             # JAM oversampling feature
    JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1                              # Custom standalone app entry point
    JUCE_VST3_CAN_REPLACE_VST2=0                                         # VST3 loads VST2-compatible state
    JUCE_STRICT_REFCOUNTEDPOINTER=1                                      # Strict ReferenceCountedObjectPtr casting
    JUCE_WEB_BROWSER=0                                                   # WebBrowserComponent support
    JUCE_USE_CURL=0                                                      # libcurl for http on Linux
    JUCE_DONT_DECLARE_PROJECTINFO=1                                      # No auto-generated ProjectInfo namespace
    VMA_VULKAN_VERSION=1002000                                           # Vulkan version for Vulkan Memory Allocator
    "$<$<CONFIG:Debug>:DEBUG=1>"                                         # Debug configuration
    "$<$<CONFIG:Release>:NDEBUG=1>"                                      # Release configuration
    "$<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN=1>"                    # Exclude rarely used Windows headers
    "$<$<PLATFORM_ID:Windows>:NOMINMAX=1>"                               # No Windows min/max macros
    "$<$<PLATFORM_ID:Windows>:_WIN32_IE=0x0A00>"                         # Windows IE platform version
    "$<$<PLATFORM_ID:Windows>:JUCE_CORE_INCLUDE_NATIVE_HEADERS=1>"       # Native platform headers in juce_core
    "$<$<PLATFORM_ID:Windows>:JUCE_GRAPHICS_INCLUDE_DIRECT2D_HELPERS=1>" # Direct2D helper visibility, from the jam patch
    "$<$<PLATFORM_ID:Windows>:JUCE_CORE_INCLUDE_COM_SMART_PTR=1>"        # Windows COM smart pointer helpers
)

# ==============================================================================
# INCLUDES — project, generated, and vendored header roots
# ==============================================================================

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Source"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/generated"
    "${CAST_USER_MODULE_PATH}/jam_freetype/freetype/include"
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
        juce_audio_utils
        juce_audio_devices
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
        "${CAST_USER_MODULE_PATH}/jam_freetype/jam_freetype.cpp"
        PROPERTIES COMPILE_FLAGS "/wd4005 /wd4804"
    )
else()
    set_source_files_properties(
        "${CAST_USER_MODULE_PATH}/jam_freetype/jam_freetype.cpp"
        "${CAST_USER_MODULE_PATH}/jam_freetype/jam_freetype.mm"
        PROPERTIES COMPILE_FLAGS "-Wno-macro-redefined"
    )
    set_source_files_properties(
        "${CAST_USER_MODULE_PATH}/jam_vulkan/jam_vulkan.cpp"
        "${CAST_USER_MODULE_PATH}/jam_vulkan/jam_vulkan.mm"
        PROPERTIES COMPILE_FLAGS "-Wno-nullability-completeness -Wno-nullability-extension -fno-strict-aliasing"
    )
endif()

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:${CAST_RELEASE_COMPILE_FLAGS}>
    $<$<CONFIG:Debug>:${CAST_DEBUG_COMPILE_FLAGS}>
)

target_link_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:${CAST_RELEASE_LINK_FLAGS}>
)

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

set(CAST_SHADER_INCLUDE_DIR "${CAST_USER_MODULE_PATH}/resources/shaders")

file(GLOB_RECURSE CAST_SHADER_SOURCES CONFIGURE_DEPENDS
    "${CAST_USER_MODULE_PATH}/resources/shaders/*.vert"
    "${CAST_USER_MODULE_PATH}/resources/shaders/*.frag"
    "${CAST_USER_MODULE_PATH}/resources/shaders/*.comp"
)

foreach(CAST_SHADER_SOURCE ${CAST_SHADER_SOURCES})
    get_filename_component(CAST_SHADER_NAME "${CAST_SHADER_SOURCE}" NAME)
    set(CAST_SHADER_SPV "${CAST_USER_MODULE_PATH}/resources/spv/${CAST_SHADER_NAME}.spv")
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

file(GLOB CAST_SHADER_SPV_FILES CONFIGURE_DEPENDS "${CAST_USER_MODULE_PATH}/resources/spv/*.spv")

if(CAST_SHADER_SPV_FILES)
    juce_add_binary_data(jam_vulkan_ShaderData
        NAMESPACE   "jam"
        HEADER_NAME "jam_VulkanShaderData.h"
        SOURCES     ${CAST_SHADER_SPV_FILES}
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE jam_vulkan_ShaderData)
endif()

file(GLOB CAST_FONT_FILES CONFIGURE_DEPENDS "${CAST_USER_MODULE_PATH}/resources/fonts/*.ttf")

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
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/layout/*.html"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/layout/*.svg"
    "${CAST_USER_MODULE_PATH}/resources/fonts/*.ttf"
)

juce_add_binary_data(${PROJECT_NAME}_BinaryData
    NAMESPACE "BinaryData"
    SOURCES ${CAST_LAYOUT_FILES}
            ${CAST_SHADER_SPV_FILES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_BinaryData)

include(CheckIPOSupported)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    check_ipo_supported(RESULT CAST_IPO_SUPPORTED OUTPUT CAST_IPO_ERROR)

    if(CAST_IPO_SUPPORTED)
        set_target_properties(${PROJECT_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)
        message(STATUS "CAST: LTO enabled")
    else()
        message(WARNING "CAST: LTO not supported — ${CAST_IPO_ERROR}")
    endif()
endif()

# ==============================================================================
# FORMATS — per-format warning flags
# ==============================================================================

foreach(CAST_FORMAT ${CAST_FORMATS})
    if(TARGET ${PROJECT_NAME}_${CAST_FORMAT})
        target_compile_options(${PROJECT_NAME}_${CAST_FORMAT} PRIVATE
            $<$<CONFIG:Release>:${CAST_RELEASE_COMPILE_FLAGS}>
            $<$<CONFIG:Debug>:${CAST_DEBUG_COMPILE_FLAGS}>
        )

        target_link_options(${PROJECT_NAME}_${CAST_FORMAT} PRIVATE
            $<$<CONFIG:Release>:${CAST_RELEASE_LINK_FLAGS}>
        )
    endif()
endforeach()

# ==============================================================================
# POST BUILD — sign, archive, install, all data-driven
# ==============================================================================

string(TIMESTAMP CAST_BUILD_DATE "%Y-%m-%d" UTC)
set(CAST_QA_PATH "$ENV{HOME}/Documents/Poems/dev/___builds___")
set(CAST_QA_DIRECTORY "${CAST_QA_PATH}/${CAST_BUILD_DATE} ${PROJECT_NAME} v${PROJECT_VERSION} macOS")

add_custom_target(post-build ALL)

set(CAST_INSTALL_DIRECTORY_Standalone "")
set(CAST_INSTALL_DIRECTORY_VST3       "$ENV{HOME}/Library/Audio/Plug-Ins/VST3")
set(CAST_INSTALL_DIRECTORY_AU         "$ENV{HOME}/Library/Audio/Plug-Ins/Components")
set(CAST_INSTALL_DIRECTORY_AAX        "/Library/Application Support/Avid/Audio/Plug-Ins")

foreach(CAST_FORMAT ${CAST_FORMATS})
    if(TARGET ${PROJECT_NAME}_${CAST_FORMAT})
        add_dependencies(post-build ${PROJECT_NAME}_${CAST_FORMAT})
    endif()
endforeach()

if(APPLE)
    foreach(CAST_FORMAT ${CAST_FORMATS})
        if(TARGET ${PROJECT_NAME}_${CAST_FORMAT})
            if(CMAKE_BUILD_TYPE STREQUAL "Release" AND CAST_SIGN)
                if(CAST_FORMAT STREQUAL "AAX")
                    add_custom_command(TARGET post-build POST_BUILD
COMMAND wraptool sign --verbose --account bayu@jrengmusic.com --wcguid 4B0468E0-A60A-11F1-BA61-005056928F3B --signid "Developer ID Application: Bayu Ardianto \\(9BDSN9TDX3\\)" --in "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}_${CAST_FORMAT}>" --out "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}_${CAST_FORMAT}>" --autoinstall on --notarize-keychain-profile notary
COMMAND ${CMAKE_COMMAND} -E make_directory "${CAST_QA_DIRECTORY}"
COMMAND ditto "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}_${CAST_FORMAT}>" "${CAST_QA_DIRECTORY}/$<TARGET_BUNDLE_DIR_NAME:${PROJECT_NAME}_${CAST_FORMAT}>"
                        COMMENT "CAST: sign ${CAST_FORMAT}"
                    )
                elseif(NOT CAST_FORMAT STREQUAL "Standalone")
                    add_custom_command(TARGET post-build POST_BUILD
COMMAND xattr -cr "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}_${CAST_FORMAT}>"
COMMAND codesign --force --options runtime --entitlements "${CMAKE_SOURCE_DIR}/entitlements.plist" --sign "Developer ID Application: Bayu Ardianto \\(9BDSN9TDX3\\)" "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}_${CAST_FORMAT}>"
COMMAND codesign --verify --verbose "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}_${CAST_FORMAT}>"
COMMAND ditto -c -k --keepParent "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}_${CAST_FORMAT}>" "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}_${CAST_FORMAT}>.zip"
COMMAND xcrun notarytool submit "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}_${CAST_FORMAT}>.zip" --keychain-profile "notary" --wait
COMMAND ${CMAKE_COMMAND} -E rm -f "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}_${CAST_FORMAT}>.zip"
COMMAND xcrun stapler staple "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}_${CAST_FORMAT}>"
COMMAND ${CMAKE_COMMAND} -E make_directory "${CAST_QA_DIRECTORY}"
COMMAND ditto "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}_${CAST_FORMAT}>" "${CAST_QA_DIRECTORY}/$<TARGET_BUNDLE_DIR_NAME:${PROJECT_NAME}_${CAST_FORMAT}>"
                        COMMENT "CAST: sign ${CAST_FORMAT}"
                    )
                endif()
            endif()

            if(CAST_INSTALL_DIRECTORY_${CAST_FORMAT})
                add_custom_command(TARGET post-build POST_BUILD
COMMAND ${CMAKE_COMMAND} -E make_directory "${CAST_INSTALL_DIRECTORY_${CAST_FORMAT}}"
COMMAND ditto "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}_${CAST_FORMAT}>" "${CAST_INSTALL_DIRECTORY_${CAST_FORMAT}}/$<TARGET_BUNDLE_DIR_NAME:${PROJECT_NAME}_${CAST_FORMAT}>"
                    COMMENT "CAST: install ${CAST_FORMAT}"
                )
            endif()
        endif()
    endforeach()
endif()

# ==============================================================================
# REPORT
# ==============================================================================

message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
message(STATUS "CAST: configuration complete")
message(STATUS "CAST: generator ${CMAKE_GENERATOR}")
message(STATUS "CAST: build directory ${CMAKE_BINARY_DIR}")
message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
