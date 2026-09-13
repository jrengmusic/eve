cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(llgc_parity
    VERSION "0.0.1"
    LANGUAGES C CXX
    DESCRIPTION "Terminal LLGC full-feature-parity proof harness"
)

set(CAST_JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../../JUCE")
set(CAST_USER_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../jam")

find_package(Git REQUIRED)

set(CAST_JUCE_VERSION 8.0.14)

set(CAST_JUCE_PATCHES
"${CAST_USER_MODULE_PATH}/patch/juce-cached-image-factory-hook.patch"       # External CachedComponentImage factory
"${CAST_USER_MODULE_PATH}/patch/juce-direct2d-helpers-visibility-hook.patch"# Direct2D helpers visibility gate
"${CAST_USER_MODULE_PATH}/patch/juce-image-subsection-hook.patch"           # Root image and subsection bounds hooks
"${CAST_USER_MODULE_PATH}/patch/juce-vulkan-engine-hook.patch"              # External graphics context factory for Vulkan
"${CAST_USER_MODULE_PATH}/patch/juce-paint-update-rect-hook.patch"          # OS dirty rectangle before native paint
"${CAST_USER_MODULE_PATH}/patch/juce-attributed-text-hook.patch"            # Attributed text virtual on LowLevelGraphicsContext
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

juce_add_module("${CAST_USER_MODULE_PATH}/jam_core")
juce_add_module("${CAST_USER_MODULE_PATH}/jam_data_structures")
juce_add_module("${CAST_USER_MODULE_PATH}/jam_graphics")
juce_add_module("${CAST_USER_MODULE_PATH}/jam_terminal")

juce_add_console_app(llgc_parity
    PRODUCT_NAME "llgc_parity"
)

juce_generate_juce_header(llgc_parity)

target_sources(llgc_parity PRIVATE main.cpp)

target_include_directories(llgc_parity PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CAST_JUCE_PATH}/modules/juce_graphics/fonts/harfbuzz"
)

target_compile_definitions(llgc_parity PRIVATE
    DONT_SET_USING_JUCE_NAMESPACE=1
    JUCE_DONT_DECLARE_PROJECTINFO=1
    LLGC_PARITY_ARTIFACTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/artifacts"
)

target_link_libraries(llgc_parity
    PRIVATE
        jam_core
        jam_data_structures
        jam_graphics
        jam_terminal
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
)
