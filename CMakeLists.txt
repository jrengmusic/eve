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

# ==============================================================================
# TOOLCHAIN — language standard, platform targets, compile database
# ==============================================================================

cmake_minimum_required(VERSION 4.2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

cmake_host_system_information(RESULT CAST_TOTAL_MEMORY QUERY TOTAL_PHYSICAL_MEMORY)
cmake_host_system_information(RESULT CAST_LOGICAL_CORES QUERY NUMBER_OF_LOGICAL_CORES)
math(EXPR CAST_COMPILE_JOBS "${CAST_TOTAL_MEMORY} / 1280")

if(CAST_COMPILE_JOBS GREATER CAST_LOGICAL_CORES)
    set(CAST_COMPILE_JOBS ${CAST_LOGICAL_CORES})
endif()

set_property(GLOBAL PROPERTY JOB_POOLS compile=${CAST_COMPILE_JOBS})
set(CMAKE_JOB_POOL_COMPILE compile)
set(CMAKE_JOB_POOL_PRECOMPILE_HEADER compile)

option(CAST_SIGN "Run the codesign/wraptool POST_BUILD chain on a Release build" ON)
set(CAST_SIGNING_PROFILE jreng CACHE STRING "Signing profile")
set_property(CACHE CAST_SIGNING_PROFILE PROPERTY STRINGS jreng)
get_property(CAST_SIGNING_PROFILES CACHE CAST_SIGNING_PROFILE PROPERTY STRINGS)
if(NOT CAST_SIGNING_PROFILE IN_LIST CAST_SIGNING_PROFILES)
    message(FATAL_ERROR "CAST: signing profile ${CAST_SIGNING_PROFILE} not in ${CAST_SIGNING_PROFILES}")
endif()
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "macOS deployment target")

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "macOS architectures")
elseif(WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set(CMAKE_RC_FLAGS "/C65001")
    set(CMAKE_NINJA_CMCLDEPS_RC OFF)
endif()

# ==============================================================================
# PROJECT
# ==============================================================================

project(EVE
    VERSION "0.1.0"
    LANGUAGES C CXX
    DESCRIPTION "EVE Audio Plugin"
)

set(CAST_FORMATS Standalone VST3 AU)

message(STATUS "CAST: configuring ${PROJECT_NAME} ${PROJECT_VERSION}")

# ==============================================================================
# WARNINGS AND OPTIMIZATION — per-platform, per-configuration flag sets
# ==============================================================================

set(CMAKE_C_FLAGS_RELEASE "")
set(CMAKE_CXX_FLAGS_RELEASE "")
set(CMAKE_C_FLAGS_DEBUG "")
set(CMAKE_CXX_FLAGS_DEBUG "")

if(APPLE)
    set(CAST_COMPILE_FLAGS
-Wno-shadow;-Wno-unused-parameter;-Wno-float-equal;-Wno-sign-conversion;-Wno-switch-enum;-Wno-implicit-float-conversion;-Wno-macro-redefined;-Wno-nullability-completeness;-Wno-nullability-extension;-fno-strict-aliasing
    )
    set(CAST_RELEASE_COMPILE_FLAGS
-O3
    )
    set(CAST_RELEASE_LINK_FLAGS
-dead_strip
    )
    set(CAST_DEBUG_COMPILE_FLAGS
-O0;-g
    )
elseif(WIN32)
    set(CAST_COMPILE_FLAGS
-Wno-shadow;-Wno-unused-parameter;-Wno-float-equal;-Wno-sign-conversion;-Wno-switch-enum;-Wno-implicit-float-conversion;/wd4005;/wd4804
    )
    set(CAST_RELEASE_COMPILE_FLAGS
/clang:-O3
    )
    set(CAST_RELEASE_LINK_FLAGS
/OPT:REF;/OPT:ICF
    )
    set(CAST_DEBUG_COMPILE_FLAGS
/Z7
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
"${CAST_USER_MODULE_PATH}/patch/juce-adhoc-sign-hook.patch"                 # No ad-hoc signature on the VST3 bundle
)

if(WIN32)
    set(CAST_JUCE_CACHE_DIR "$ENV{LOCALAPPDATA}/cast/juce")
else()
    set(CAST_JUCE_CACHE_DIR "$ENV{HOME}/.cache/cast/juce")
endif()

set(CAST_JUCE_PATCH_SET_CONCAT "${CAST_JUCE_VERSION}")
foreach(CAST_JUCE_PATCH ${CAST_JUCE_PATCHES})
    file(READ "${CAST_JUCE_PATCH}" CAST_JUCE_PATCH_CONTENT)
    string(APPEND CAST_JUCE_PATCH_SET_CONCAT "${CAST_JUCE_PATCH_CONTENT}")
endforeach()

set(CAST_JUCE_PATCH_SET_CONCAT_FILE "${CMAKE_BINARY_DIR}/cast_juce_patch_set_concat.tmp")
file(WRITE "${CAST_JUCE_PATCH_SET_CONCAT_FILE}" "${CAST_JUCE_PATCH_SET_CONCAT}")
file(SHA256 "${CAST_JUCE_PATCH_SET_CONCAT_FILE}" CAST_JUCE_PATCH_SET_HASH)
file(REMOVE "${CAST_JUCE_PATCH_SET_CONCAT_FILE}")

set(CAST_PATCHED_JUCE_PATH "${CAST_JUCE_CACHE_DIR}/${CAST_JUCE_PATCH_SET_HASH}")
set(CAST_JUCE_PATCH_STAMP_FILE "${CAST_PATCHED_JUCE_PATH}/.patch-stamp")

if(NOT EXISTS "${CAST_JUCE_PATCH_STAMP_FILE}")
    message(STATUS "CAST: patching JUCE into ${CAST_PATCHED_JUCE_PATH}")

    if(EXISTS "${CAST_PATCHED_JUCE_PATH}")
        file(REMOVE_RECURSE "${CAST_PATCHED_JUCE_PATH}")
    endif()

    file(REAL_PATH "${CAST_JUCE_PATH}" CAST_JUCE_PATH)

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

set(CAST_JUCE_PATCH_SET_STAMP_FILE "${CMAKE_BINARY_DIR}/cast_juce_patch_set")

if(EXISTS "${CAST_JUCE_PATCH_SET_STAMP_FILE}")
    file(READ "${CAST_JUCE_PATCH_SET_STAMP_FILE}" CAST_BUILT_JUCE_PATCH_SET_HASH)
endif()

if(NOT "${CAST_BUILT_JUCE_PATCH_SET_HASH}" STREQUAL "${CAST_JUCE_PATCH_SET_HASH}")
    file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/JUCE" "${CMAKE_BINARY_DIR}/vst3_helpers")
    file(WRITE "${CAST_JUCE_PATCH_SET_STAMP_FILE}" "${CAST_JUCE_PATCH_SET_HASH}")
endif()

add_subdirectory("${CAST_JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE")

get_directory_property(CAST_FOUND_JUCE_VERSION DIRECTORY "${CAST_JUCE_PATH}" DEFINITION JUCE_VERSION)

if(NOT CAST_FOUND_JUCE_VERSION VERSION_EQUAL CAST_JUCE_VERSION)
    message(FATAL_ERROR "CAST: JUCE ${CAST_FOUND_JUCE_VERSION} found, ${CAST_JUCE_VERSION} required — exact match")
endif()

message(STATUS "CAST: JUCE ${CAST_FOUND_JUCE_VERSION} patched at ${CAST_JUCE_PATH}")

if("VST" IN_LIST CAST_FORMATS)
    juce_set_vst2_sdk_path("${CAST_USER_MODULE_PATH}/sdk/VST2")
endif()

# ==============================================================================
# MODULES — JAM framework modules
# ==============================================================================

juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_core")             # JAM Core
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_debug")            # Debug instruments — console, log, model monitor
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_data_structures")  # ValueTree management and data model utilities — model, parameters, JSON conversion
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_dsp")              # DSP processors — filters, waveshaping, hysteresis modelling, transient control, oversampling, FIR, noise generation, spectrum analysis
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_gui")              # GUI foundation — Window, Modal, Glass
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_graphics")         # Graphics utilities, blur, shadows, colours, fonts, mesh
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_animation")        # Foundation animation classes (Animator, AnimationBase, AnimationScrollingText)
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_freetype")         # Vendored FreeType font rasterization library
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_vulkan")           # Vulkan rendering backend
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_style")            # JAM Style — LookAndFeel base + ColourScheme-backed colour registry
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_markdown")         # Clean-room native CommonMark + GFM markdown parsing and rendering
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_document")         # Universal line break and reflow over jam::Document (UAX #14)
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_web")              # HTML authored-subset and CSS Syntax Level 3 subset tokenizers and parsers
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_plugin_bootstrap") # Document-driven plugin bootstrap — view construction, style management, plugin editor base, standalone shell
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_pdf")              # Clean-room PDF document model, writer and graphics context
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_mermaid_diagram")  # Clean-room native mermaid diagram parsing over jam::Document producing the semantic Element tree
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_markdown_graphics")# Markdown and mermaid materialisation, layout and view component

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
    BUNDLE_ID com.jreng.EVE
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Source/icons/end-icon.png"

    FORMATS ${CAST_FORMATS}
    VST3_CATEGORIES Fx
    AAX_CATEGORY AAX_ePlugInCategory_Effect

    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE

    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
    Source/EVEProcessor.h            # juce::AudioProcessor plugin entry class declaration
    Source/EVEProcessor.cpp          # juce::AudioProcessor plugin entry, state serialization, createPluginFilter
    Source/EVEAudioProcessor.h       # Audio processor class declaration
    Source/EVEView.h                 # Plugin editor class declaration, jam::PluginEditor
    Source/EVEView.cpp               # Plugin editor — theme, component registry, panel and view construction
    Source/EVEViewRegistration.cpp   # Plugin editor button registration and dialog config
    Source/EVEViewComponents.cpp     # Plugin editor view component configs
    Source/EVEViewPanelCallbacks.cpp # Plugin editor panel, about and settings dialog callbacks
    Source/generated/Identifiers.cpp # Product identifier vocabulary definitions
    Source/generated/Files.cpp       # Product asset file name definitions
)

target_precompile_headers(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:<JuceHeader.h$<ANGLE-R>>")

file(GLOB CAST_MODULE_SOURCES
    "${CAST_JUCE_PATH}/modules/*/*.cpp"
    "${CAST_JUCE_PATH}/modules/*/*.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/../jam/*/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../jam/*/*.mm"
)

set_source_files_properties(${CAST_MODULE_SOURCES} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

# ==============================================================================
# DEFINES — one call, platform and configuration routed by generator expression
# ==============================================================================

target_compile_definitions(${PROJECT_NAME} PUBLIC
    DONT_SET_USING_JUCE_NAMESPACE=1                                      # No using namespace juce in JuceHeader.h
    JAM_USING_OVERSAMPLING=0                                             # JAM oversampling feature
    JAM_USING_MULTI_ORIENTATION=0                                        # JAM portrait/landscape orientation
    JAM_VULKAN_RUNTIME_SHADER_COMPILER=0                                 # JAM runtime SPIR-V shader compilation
    JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1                              # Custom standalone app entry point
    JUCE_VST3_CAN_REPLACE_VST2=0                                         # VST3 loads VST2-compatible state
    JucePlugin_AAXDisableDefaultSettingsChunks=1                         # AAX default settings chunks disabled
    JUCE_STRICT_REFCOUNTEDPOINTER=1                                      # Strict ReferenceCountedObjectPtr casting
    JUCE_WEB_BROWSER=0                                                   # WebBrowserComponent support
    JUCE_USE_CURL=0                                                      # libcurl for http on Linux
    JUCE_DONT_DECLARE_PROJECTINFO=1                                      # No auto-generated ProjectInfo namespace
    VMA_VULKAN_VERSION=1002000                                           # Vulkan version for Vulkan Memory Allocator
    "$<$<CONFIG:Debug>:DEBUG=1>"                                         # Debug configuration
    "$<$<CONFIG:Release>:NDEBUG=1>"                                      # Release configuration
    "$<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN=1>"                    # Exclude rarely used Windows headers
    "$<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS=1>"                # C runtime deprecation warnings off
    "$<$<PLATFORM_ID:Windows>:WINVER=_WIN32_WINNT_WIN10>"                # Windows API version, same as JUCE
    "$<$<PLATFORM_ID:Windows>:_WIN32_IE=0x0A00>"                         # Windows IE platform version
    "$<$<PLATFORM_ID:Windows>:JUCE_CORE_INCLUDE_NATIVE_HEADERS=1>"       # Native platform headers in juce_core
    "$<$<PLATFORM_ID:Windows>:JUCE_GRAPHICS_INCLUDE_DIRECT2D_HELPERS=1>" # Direct2D helper visibility, from the JAM patch
    "$<$<PLATFORM_ID:Windows>:JUCE_CORE_INCLUDE_COM_SMART_PTR=1>"        # Windows COM smart pointer helpers
)

# ==============================================================================
# INCLUDES — project, generated, and vendored header roots
# ==============================================================================

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Source"                     # Project source headers
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/generated"           # Generated project headers
    "${CAST_USER_MODULE_PATH}/jam_freetype/freetype/include" # Vendored FreeType public headers
    "${CAST_JUCE_PATH}/modules/juce_graphics/fonts/harfbuzz" # JUCE-bundled HarfBuzz headers
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
        juce_dsp
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
        jam_document
        jam_web
        jam_plugin_bootstrap
        jam_pdf
        jam_mermaid_diagram
        jam_markdown_graphics
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

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        find_file(CAST_VULKAN_VALIDATION_LAYER_MANIFEST VkLayer_khronos_validation.json
            PATHS "${CAST_VULKAN_PATH}/Bin"
            NO_DEFAULT_PATH
            REQUIRED)
        file(TO_NATIVE_PATH "${CAST_VULKAN_VALIDATION_LAYER_MANIFEST}" CAST_VULKAN_VALIDATION_LAYER_MANIFEST)

        execute_process(
            COMMAND reg query "HKLM\\SOFTWARE\\Khronos\\Vulkan\\ExplicitLayers" /v "${CAST_VULKAN_VALIDATION_LAYER_MANIFEST}"
            RESULT_VARIABLE CAST_VULKAN_VALIDATION_LAYER_RESULT
            OUTPUT_QUIET
            ERROR_QUIET
        )

        if(NOT CAST_VULKAN_VALIDATION_LAYER_RESULT EQUAL 0)
            execute_process(
                COMMAND sudo reg add "HKLM\\SOFTWARE\\Khronos\\Vulkan\\ExplicitLayers" /v "${CAST_VULKAN_VALIDATION_LAYER_MANIFEST}" /t REG_DWORD /d 0 /f
            )
            execute_process(
                COMMAND reg query "HKLM\\SOFTWARE\\Khronos\\Vulkan\\ExplicitLayers" /v "${CAST_VULKAN_VALIDATION_LAYER_MANIFEST}"
                RESULT_VARIABLE CAST_VULKAN_VALIDATION_LAYER_RESULT
                OUTPUT_QUIET
                ERROR_QUIET
            )

            if(NOT CAST_VULKAN_VALIDATION_LAYER_RESULT EQUAL 0)
                message(FATAL_ERROR "CAST: Vulkan validation layer not registered: ${CAST_VULKAN_VALIDATION_LAYER_MANIFEST}")
            endif()
        endif()
    endif()
endif()

target_compile_options(${PROJECT_NAME} PRIVATE
    ${CAST_COMPILE_FLAGS}
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
    list(APPEND CAST_SHADER_SPV_OUTPUTS "${CAST_SHADER_SPV}")
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
list(APPEND CAST_SHADER_SPV_FILES ${CAST_SHADER_SPV_OUTPUTS})
list(REMOVE_DUPLICATES CAST_SHADER_SPV_FILES)

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
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/layout/*.md"   # Component and parameter markdown tables
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/layout/*.css"  # View stylesheets
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/layout/*.xml"  # Default settings XML
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/layout/*.html" # Panel, settings, and about HTML layouts
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/layout/*.svg"  # View layout and about graphics SVG
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/images/*.png"  # Product PNG images — knobs, faders, toggles, backgrounds
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/buttons/*.svg" # Product SVG button states — normal, over, down
    "${CAST_USER_MODULE_PATH}/resources/svg/*.svg"     # Shared JAM SVG icons
    "${CAST_USER_MODULE_PATH}/resources/fonts/*.ttf"   # Shared JAM Display and DisplayMono TrueType fonts
)

file(GLOB CAST_BINARY_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../jam/resources/markdown.css
    ${CMAKE_CURRENT_SOURCE_DIR}/../jam/resources/mermaid.css
)

juce_add_binary_data(${PROJECT_NAME}_BinaryData
    NAMESPACE "BinaryData"
    SOURCES ${CAST_LAYOUT_FILES}
            ${CAST_SHADER_SPV_FILES}
            ${CAST_BINARY_FILES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_BinaryData)

include(CheckIPOSupported)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    check_ipo_supported(RESULT CAST_IPO_SUPPORTED OUTPUT CAST_IPO_ERROR)

    if(CAST_IPO_SUPPORTED)
        set_target_properties(${PROJECT_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)

        foreach(CAST_FORMAT ${CAST_FORMATS})
            if(TARGET ${PROJECT_NAME}_${CAST_FORMAT})
                set_target_properties(${PROJECT_NAME}_${CAST_FORMAT} PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)
            endif()
        endforeach()

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
            ${CAST_COMPILE_FLAGS}
            $<$<CONFIG:Release>:${CAST_RELEASE_COMPILE_FLAGS}>
            $<$<CONFIG:Debug>:${CAST_DEBUG_COMPILE_FLAGS}>
        )

        target_link_options(${PROJECT_NAME}_${CAST_FORMAT} PRIVATE
            $<$<CONFIG:Release>:${CAST_RELEASE_LINK_FLAGS}>
        )
    endif()
endforeach()

# ==============================================================================
# POST BUILD — sign, install, pack, all data-driven
# ==============================================================================

if(APPLE)
    set(CAST_INSTALL_DIRECTORY_Standalone "/Applications")
elseif(WIN32)
    set(CAST_INSTALL_DIRECTORY_Standalone [==[$PROGRAMFILES64\JRENG]==])
endif()
if(APPLE)
    set(CAST_INSTALL_DIRECTORY_VST3 "/Library/Audio/Plug-Ins/VST3")
elseif(WIN32)
    set(CAST_INSTALL_DIRECTORY_VST3 [==[$COMMONFILES64\VST3]==])
endif()
if(APPLE)
    set(CAST_INSTALL_DIRECTORY_AU "/Library/Audio/Plug-Ins/Components")
elseif(WIN32)
    set(CAST_INSTALL_DIRECTORY_AU [==[]==])
endif()

if(WIN32)
    foreach(CAST_FORMAT ${CAST_FORMATS})
        string(REPLACE "$PROGRAMFILES64" "$ENV{ProgramW6432}" CAST_INSTALL_DIRECTORY_${CAST_FORMAT} "${CAST_INSTALL_DIRECTORY_${CAST_FORMAT}}")
        string(REPLACE "$COMMONFILES64" "$ENV{CommonProgramW6432}" CAST_INSTALL_DIRECTORY_${CAST_FORMAT} "${CAST_INSTALL_DIRECTORY_${CAST_FORMAT}}")
        file(TO_CMAKE_PATH "${CAST_INSTALL_DIRECTORY_${CAST_FORMAT}}" CAST_INSTALL_DIRECTORY_${CAST_FORMAT})
    endforeach()
endif()

if(CAST_SIGNING_PROFILE STREQUAL "jreng")
    set(CAST_SIGNING_identity "Developer ID Application: Bayu Ardianto \\(9BDSN9TDX3\\)")
endif()
if(CAST_SIGNING_PROFILE STREQUAL "jreng")
    set(CAST_SIGNING_identityWindows "8274AC1EA9DEA10AEFF7FD683D5E2BC4B5FAAAF1")
endif()
if(CAST_SIGNING_PROFILE STREQUAL "jreng")
    set(CAST_SIGNING_certificateWindows "")
endif()
if(CAST_SIGNING_PROFILE STREQUAL "jreng")
    set(CAST_SIGNING_entitlementsPath "entitlements.plist")
endif()
if(CAST_SIGNING_PROFILE STREQUAL "jreng")
    set(CAST_SIGNING_edenAccount "bayu@jrengmusic.com")
endif()
if(CAST_SIGNING_PROFILE STREQUAL "jreng")
    set(CAST_SIGNING_edenPassword "")
endif()
if(CAST_SIGNING_PROFILE STREQUAL "jreng")
    set(CAST_SIGNING_wrapConfigGuid "4B0468E0-A60A-11F1-BA61-005056928F3B")
endif()
if(CAST_SIGNING_PROFILE STREQUAL "jreng")
    set(CAST_SIGNING_signArgumentsWindows "sign /sha1 ${CAST_SIGNING_identityWindows} /fd SHA256")
endif()
if(CAST_SIGNING_PROFILE STREQUAL "jreng")
    set(CAST_SIGNING_signFormatsWindows "")
endif()
if(CAST_SIGNING_PROFILE STREQUAL "jreng")
    set(CAST_SIGNING_signFormats "Standalone;VST3;AU")
endif()
if(CAST_SIGNING_PROFILE STREQUAL "jreng")
    set(CAST_SIGNING_installFormats "VST3;AU")
endif()

if(NOT "${CAST_SIGNING_edenPassword}" STREQUAL "")
    set(CAST_SIGNING_WRAPTOOL_ACCOUNT --account ${CAST_SIGNING_edenAccount} --password ${CAST_SIGNING_edenPassword})
else()
    set(CAST_SIGNING_WRAPTOOL_ACCOUNT --account ${CAST_SIGNING_edenAccount})
endif()
if(WIN32 AND CAST_SIGN AND CMAKE_BUILD_TYPE STREQUAL "Release")
    cmake_host_system_information(RESULT CAST_SIGNTOOL_KITS_ROOT QUERY WINDOWS_REGISTRY "HKLM/SOFTWARE/Microsoft/Windows Kits/Installed Roots" VALUE KitsRoot10 VIEW 64_32)
    file(GLOB CAST_SIGNTOOL_KITS_VERSIONS LIST_DIRECTORIES true "${CAST_SIGNTOOL_KITS_ROOT}bin/10.*")
    list(SORT CAST_SIGNTOOL_KITS_VERSIONS COMPARE NATURAL ORDER DESCENDING)
    find_program(CAST_SIGNTOOL signtool PATHS ${CAST_SIGNTOOL_KITS_VERSIONS} PATH_SUFFIXES x64 NO_DEFAULT_PATH NO_CACHE REQUIRED)
    file(TO_NATIVE_PATH "${CAST_SIGNTOOL}" CAST_SIGNTOOL)
    if(NOT "${CAST_SIGNING_certificateWindows}" STREQUAL "")
        set(CAST_SIGNING_WRAPTOOL_OPTIONS --signtool "${CAST_SIGNTOOL}" --explicitsigningoptions "${CAST_SIGNING_signArgumentsWindows}")
    else()
        set(CAST_SIGNING_WRAPTOOL_OPTIONS --signid "${CAST_SIGNING_identityWindows}")
    endif()
    separate_arguments(CAST_SIGNING_ARGUMENTS WINDOWS_COMMAND "${CAST_SIGNING_signArgumentsWindows}")
endif()

if(APPLE)
    foreach(CAST_FORMAT ${CAST_FORMATS})
        if(TARGET ${PROJECT_NAME}_${CAST_FORMAT})
            if(CMAKE_BUILD_TYPE STREQUAL "Release" AND CAST_SIGN)
                if(CAST_FORMAT STREQUAL "AAX")
                    add_custom_command(TARGET ${PROJECT_NAME}_${CAST_FORMAT} POST_BUILD
COMMAND wraptool sign --verbose ${CAST_SIGNING_WRAPTOOL_ACCOUNT} --wcguid ${CAST_SIGNING_wrapConfigGuid} --signid "${CAST_SIGNING_identity}" --in "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}_${CAST_FORMAT}>" --out "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}_${CAST_FORMAT}>" --autoinstall on
                        COMMENT "CAST: sign ${CAST_FORMAT}"
                        USES_TERMINAL
                    )
                elseif(CAST_FORMAT IN_LIST CAST_SIGNING_signFormats)
                    add_custom_command(TARGET ${PROJECT_NAME}_${CAST_FORMAT} POST_BUILD
COMMAND xattr -cr "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}_${CAST_FORMAT}>"
COMMAND codesign --force --options runtime --entitlements "${CMAKE_SOURCE_DIR}/${CAST_SIGNING_entitlementsPath}" --sign "${CAST_SIGNING_identity}" "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}_${CAST_FORMAT}>"
COMMAND codesign --verify --verbose "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}_${CAST_FORMAT}>"
                        COMMENT "CAST: sign ${CAST_FORMAT}"
                    )
                endif()
            endif()

            if(CAST_INSTALL_DIRECTORY_${CAST_FORMAT} AND CAST_FORMAT IN_LIST CAST_SIGNING_installFormats)
                add_custom_command(TARGET ${PROJECT_NAME}_${CAST_FORMAT} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E make_directory "${CAST_INSTALL_DIRECTORY_${CAST_FORMAT}}"
COMMAND ditto "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}_${CAST_FORMAT}>" "${CAST_INSTALL_DIRECTORY_${CAST_FORMAT}}/$<TARGET_BUNDLE_DIR_NAME:${PROJECT_NAME}_${CAST_FORMAT}>"
                    COMMENT "CAST: install ${CAST_FORMAT}"
                )
            endif()
        endif()
    endforeach()
elseif(WIN32)
    foreach(CAST_FORMAT ${CAST_FORMATS})
        if(TARGET ${PROJECT_NAME}_${CAST_FORMAT})
            if(CMAKE_BUILD_TYPE STREQUAL "Release" AND CAST_SIGN)
                if(CAST_FORMAT STREQUAL "AAX")
                    add_custom_command(TARGET ${PROJECT_NAME}_${CAST_FORMAT} POST_BUILD
COMMAND wraptool sign --verbose ${CAST_SIGNING_WRAPTOOL_ACCOUNT} --wcguid ${CAST_SIGNING_wrapConfigGuid} ${CAST_SIGNING_WRAPTOOL_OPTIONS} --in "$<TARGET_FILE_DIR:${PROJECT_NAME}_${CAST_FORMAT}>/../.." --out "$<TARGET_FILE_DIR:${PROJECT_NAME}_${CAST_FORMAT}>/../.." --autoinstall on
                        COMMENT "CAST: sign ${CAST_FORMAT}"
                        VERBATIM
                        USES_TERMINAL
                    )
                elseif(CAST_FORMAT IN_LIST CAST_SIGNING_signFormatsWindows)
                    add_custom_command(TARGET ${PROJECT_NAME}_${CAST_FORMAT} POST_BUILD
COMMAND "${CAST_SIGNTOOL}" ${CAST_SIGNING_ARGUMENTS} "$<TARGET_FILE:${PROJECT_NAME}_${CAST_FORMAT}>"
                        COMMENT "CAST: sign ${CAST_FORMAT}"
                        VERBATIM
                        USES_TERMINAL
                    )
                endif()
            endif()

            if(CAST_INSTALL_DIRECTORY_${CAST_FORMAT} AND CAST_FORMAT IN_LIST CAST_SIGNING_installFormats)
                if(CAST_FORMAT STREQUAL "VST")
                    add_custom_command(TARGET ${PROJECT_NAME}_${CAST_FORMAT} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E make_directory "${CAST_INSTALL_DIRECTORY_${CAST_FORMAT}}"
COMMAND sudo "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:${PROJECT_NAME}_${CAST_FORMAT}>" "${CAST_INSTALL_DIRECTORY_${CAST_FORMAT}}"
                        COMMENT "CAST: install ${CAST_FORMAT}"
                    )
                else()
                    add_custom_command(TARGET ${PROJECT_NAME}_${CAST_FORMAT} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E make_directory "${CAST_INSTALL_DIRECTORY_${CAST_FORMAT}}"
COMMAND sudo "${CMAKE_COMMAND}" -E copy_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}_${CAST_FORMAT}>/../.." "${CAST_INSTALL_DIRECTORY_${CAST_FORMAT}}/$<TARGET_PROPERTY:${PROJECT_NAME},JUCE_PRODUCT_NAME>.$<TARGET_PROPERTY:${PROJECT_NAME}_${CAST_FORMAT},BUNDLE_EXTENSION>"
                        COMMENT "CAST: install ${CAST_FORMAT}"
                    )
                endif()
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
