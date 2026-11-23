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

cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

cmake_host_system_information(RESULT CAST_TOTAL_MEMORY QUERY TOTAL_PHYSICAL_MEMORY)
cmake_host_system_information(RESULT CAST_LOGICAL_CORES QUERY NUMBER_OF_LOGICAL_CORES)
cmake_host_system_information(RESULT CAST_HOST_PROCESSOR QUERY OS_PLATFORM)
math(EXPR CAST_COMPILE_JOBS "${CAST_TOTAL_MEMORY} / 1280")

if(CAST_COMPILE_JOBS GREATER CAST_LOGICAL_CORES)
    set(CAST_COMPILE_JOBS ${CAST_LOGICAL_CORES})
endif()

set_property(GLOBAL PROPERTY JOB_POOLS compile=${CAST_COMPILE_JOBS})
set(CMAKE_JOB_POOL_COMPILE compile)
set(CMAKE_JOB_POOL_PRECOMPILE_HEADER compile)

set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "macOS deployment target")

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "${CAST_HOST_PROCESSOR}" CACHE STRING "macOS architectures")
elseif(WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set(CMAKE_RC_FLAGS "/C65001")
    set(CMAKE_NINJA_CMCLDEPS_RC OFF)
endif()

project(eve-cygwinPTY
    VERSION "0.0.1"
    LANGUAGES C CXX
    DESCRIPTION "MSYS2 and Cygwin pty helper of EVE"
)

find_program(CAST_CCACHE ccache NO_CACHE REQUIRED)
get_property(CAST_LANGUAGES GLOBAL PROPERTY ENABLED_LANGUAGES)

foreach(CAST_LANGUAGE ${CAST_LANGUAGES})
    set(CMAKE_${CAST_LANGUAGE}_COMPILER_LAUNCHER "${CAST_CCACHE}")
endforeach()

message(STATUS "\neve-cygwinPTY")

execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE CAST_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "CAST: configuring ${PROJECT_NAME} ${PROJECT_VERSION}")

set(CMAKE_C_FLAGS_RELEASE "")
set(CMAKE_CXX_FLAGS_RELEASE "")
set(CMAKE_C_FLAGS_DEBUG "")
set(CMAKE_CXX_FLAGS_DEBUG "")

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
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(CAST_RELEASE_COMPILE_FLAGS
/wd4456;/wd4100;/wd4244;/permissive-;/Zc:rvalueCast;/W4;/FC;/O2;/GL
        )
        set(CAST_RELEASE_LINK_FLAGS
/LTCG;/OPT:REF;/OPT:ICF
        )
        set(CAST_DEBUG_COMPILE_FLAGS
/wd4456;/wd4100;/wd4244;/permissive-;/Zc:rvalueCast;/W4;/FC;/Od;/Zi
        )
    else()
        set(CAST_RELEASE_COMPILE_FLAGS
-Wno-shadow;-Wno-unused-parameter;-Wno-float-equal;-Wno-sign-conversion;-Wno-switch-enum;-Wno-implicit-float-conversion;-O3;-flto=thin
        )
        list(REMOVE_ITEM CAST_RELEASE_COMPILE_FLAGS "-O3")
        list(APPEND CAST_RELEASE_COMPILE_FLAGS "/clang:-O3")
        set(CAST_RELEASE_LINK_FLAGS
/LTCG;/OPT:REF;/OPT:ICF
        )
        set(CAST_DEBUG_COMPILE_FLAGS
-Wno-shadow;-Wno-unused-parameter;-Wno-float-equal;-Wno-sign-conversion;-Wno-switch-enum;-Wno-implicit-float-conversion;-O0;-g
        )
        list(REMOVE_ITEM CAST_DEBUG_COMPILE_FLAGS "-O0" "-g")
        list(APPEND CAST_DEBUG_COMPILE_FLAGS "/Z7")
    endif()
endif()

set(CAST_JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../JUCE")
set(CAST_USER_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../jam")

set(CAST_JUCE_VERSION 8.0.17)

find_package(Git REQUIRED)

set(CAST_JUCE_PATCHES
"${CAST_USER_MODULE_PATH}/patch/juce-cached-image-factory-hook.patch"       # External CachedComponentImage factory
"${CAST_USER_MODULE_PATH}/patch/juce-direct2d-helpers-visibility-hook.patch"# Direct2D helpers visibility gate
"${CAST_USER_MODULE_PATH}/patch/juce-image-subsection-hook.patch"           # Root image, subsection bounds, and virtual ImagePixelData listener hooks
"${CAST_USER_MODULE_PATH}/patch/juce-vulkan-engine-hook.patch"              # External graphics context factory for Vulkan; HWNDComponentPeer skips Win32 WS_EX_LAYERED while externalContextFactory is active and restores layered state on fallback to the default renderer; invalidates and validates with RDW_NOCHILDREN so child windows keep their pending paint
"${CAST_USER_MODULE_PATH}/patch/juce-paint-update-rect-hook.patch"          # OS dirty rectangle before native paint; applies after juce-vulkan-engine-hook.patch
"${CAST_USER_MODULE_PATH}/patch/juce-attributed-text-hook.patch"            # Attributed text virtual on LowLevelGraphicsContext; applies after juce-vulkan-engine-hook.patch
"${CAST_USER_MODULE_PATH}/patch/juce-adhoc-sign-hook.patch"                 # No ad-hoc signature on the VST3 bundle
"${CAST_USER_MODULE_PATH}/patch/juce-display-changed-hook.patch"            # Callback after refresh() finds a display change
"${CAST_USER_MODULE_PATH}/patch/juce-plugin-transparency-hook.patch"        # Plugin editor wrappers (VST2, VST3) transparent when the host is END; host plugin windows transparent; applies after juce-vulkan-engine-hook.patch
"${CAST_USER_MODULE_PATH}/patch/juce-key-pressed-hook.patch"                # Host key press callback before a peer's components; applies after juce-plugin-transparency-hook.patch
"${CAST_USER_MODULE_PATH}/patch/juce-plugin-host-type-hook.patch"           # END registered as a known PluginHostType host: HostType END, isEND(), description, host file name detection
"${CAST_USER_MODULE_PATH}/patch/juce-plugin-window-focus-hook.patch"        # VST2 host plugin windows take keyboard focus and hand focus to the native plugin view (macOS first responder, Windows SetFocus, Linux XEmbedComponent), and AU host plugin windows make the plugin view first responder; HWNDComponentPeer treats a focused child HWND as focused and skips focus loss on child-HWND focus handoff; adds handleFocusLoss after SetFocus on Windows and clears mouseActivateFlags in WM_KILLFOCUS; applies after juce-plugin-transparency-hook.patch
"${CAST_USER_MODULE_PATH}/patch/juce-mouse-event-hook.patch"                # Host mouse event callback before a peer's components; click grab only when no external mouse callback is set or the peer is focused; applies after juce-key-pressed-hook.patch
"${CAST_USER_MODULE_PATH}/patch/juce-plugin-editor-once-hook.patch"         # VST2 plugin client opens the editor that effEditGetRect created, so the editor is built once; applies after juce-plugin-transparency-hook.patch
"${CAST_USER_MODULE_PATH}/patch/juce-plugin-window-size-hook.patch"         # VST2 host child window takes the plugin's effEditGetRect size in physical pixels on Windows; applies after juce-plugin-transparency-hook.patch
"${CAST_USER_MODULE_PATH}/patch/juce-value-tree-synchroniser-hook.patch"    # Listener exclusion for ValueTree senders: juce_ValueTree.cpp, juce_ValueTree.h, juce_ValueTreeSynchroniser.cpp, juce_ValueTreeSynchroniser.h; child and remove-property senders take a listener to exclude; ValueTree gains removePropertyExcludingListener, addChildExcludingListener, removeChildExcludingListener, moveChildExcludingListener; ValueTreeSynchroniser gains applyChange that excludes itself
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

juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../../jam/jam_core")           # JAM Core
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../../jam/jam_document")       # Universal line break and reflow (UAX #14)
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../../jam/jam_freetype")       # Vendored FreeType font rasterization library
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../../jam/jam_graphics")       # Graphics utilities, blur, shadows, colours, fonts, mesh
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../../jam/jam_data_structures")# ValueTree management and data model utilities — model, parameters
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../../jam/jam_web")            # HTML authored-subset and CSS Syntax Level 3 subset tokenizers and parsers
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../../jam/jam_style")          # JAM Style — LookAndFeel base + ColourScheme-backed colour registry
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../../jam/jam_terminal")       # VT read-side engine, TTY, protocol vocabulary, keyboard encoding

message(STATUS "CAST: user modules added from ${CMAKE_CURRENT_SOURCE_DIR}/../../jam")

juce_add_console_app(${PROJECT_NAME}
    PRODUCT_NAME "eve-cygwinPTY"
    VERSION "0.0.1"
    BUILD_VERSION "0.1.0"
    BUNDLE_ID "com.jrengmusic.eve.cygwinPTY"
    COMPANY_NAME "JRENG"
    COMPANY_COPYRIGHT "© 2026 JRENG"
    COMPANY_WEBSITE "https://jrengmusic.com"
    COMPANY_EMAIL "info@jrengmusic.com"
    DOCUMENT_EXTENSIONS 
    NEEDS_CURL OFF
    NEEDS_WEB_BROWSER OFF
    NEEDS_WEBVIEW2 OFF
    NEEDS_STORE_KIT OFF
)

juce_generate_juce_header(${PROJECT_NAME})

file(GLOB_RECURSE CAST_SOURCE_FILES CONFIGURE_DEPENDS
${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h
)

target_sources(${PROJECT_NAME} PRIVATE ${CAST_SOURCE_FILES})

target_precompile_headers(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:<JuceHeader.h$<ANGLE-R>>")

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE "SHELL:-Xclang -fno-pch-timestamp")
endif()

file(GLOB CAST_MODULE_SOURCES
    "${CAST_JUCE_PATH}/modules/*/*.cpp"
    "${CAST_JUCE_PATH}/modules/*/*.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../jam/*/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../jam/*/*.mm"
)

set_source_files_properties(${CAST_MODULE_SOURCES} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

target_compile_definitions(${PROJECT_NAME} PUBLIC
    CAST_COMMIT="${CAST_COMMIT_HASH}"
DONT_SET_USING_JUCE_NAMESPACE=1
JUCE_DONT_DECLARE_PROJECTINFO=1
"$<$<PLATFORM_ID:Windows>:_WIN32_IE=0x0A00>"
"$<$<PLATFORM_ID:Windows>:JUCE_CORE_INCLUDE_NATIVE_HEADERS=1>"
"$<$<PLATFORM_ID:Windows>:JUCE_CORE_INCLUDE_COM_SMART_PTR=1>"
)

target_include_directories(${PROJECT_NAME} PRIVATE
${CMAKE_CURRENT_SOURCE_DIR}/../../jam
${CMAKE_CURRENT_SOURCE_DIR}/Source/generated
"${CAST_USER_MODULE_PATH}/jam_freetype/freetype/include"
"${CAST_JUCE_PATH}/modules/juce_graphics/fonts/harfbuzz"
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
juce_core
juce_data_structures
juce_events
juce_graphics
juce_gui_basics
juce_audio_processors
jam_core
jam_document
jam_freetype
jam_graphics
jam_data_structures
jam_web
jam_style
jam_terminal
    PUBLIC
        juce::juce_recommended_config_flags
)

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
endif()

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:${CAST_RELEASE_COMPILE_FLAGS}>
    $<$<CONFIG:Debug>:${CAST_DEBUG_COMPILE_FLAGS}>
)

target_link_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:${CAST_RELEASE_LINK_FLAGS}>
)

if(MSVC)
    target_link_options(${PROJECT_NAME} PRIVATE "/ENTRY:cygwinPTYCRTStartup")
endif()

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
# POST BUILD — pack
# ==============================================================================

if(APPLE)
    set(CAST_PACK_folder "../Source/windows")
elseif(WIN32)
    set(CAST_PACK_folder "../Source/windows")
endif()
if(APPLE)
    set(CAST_PACK_format "zip")
elseif(WIN32)
    set(CAST_PACK_format "zip")
endif()
if(APPLE)
    set(CAST_PACK_platform "macOS")
elseif(WIN32)
    set(CAST_PACK_platform "Windows")
endif()

set(CAST_PACK_NAME "eve-cygwinPTY v${PROJECT_VERSION} ${CAST_PACK_platform}")
set(CAST_PACK_ARCHIVE "${CMAKE_SOURCE_DIR}/${CAST_PACK_folder}/${CAST_PACK_NAME}.${CAST_PACK_format}")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND cast --pack "${CAST_PACK_ARCHIVE}" "$<TARGET_FILE:${PROJECT_NAME}>"
    COMMENT "CAST: pack ${CAST_PACK_ARCHIVE}"
    VERBATIM
    USES_TERMINAL
)

message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
message(STATUS "CAST: ${PROJECT_NAME} configured")
message(STATUS "Generator: ${CMAKE_GENERATOR}")
message(STATUS "Build directory: ${CMAKE_BINARY_DIR}")
message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
