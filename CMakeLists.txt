cmake_minimum_required(VERSION 4.2.0)

set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "macOS deployment target")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "macOS architectures")
elseif(WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE OR UNIX)
    add_compile_options(
        -Wno-float-equal
        -Wno-sign-conversion
        -Wno-implicit-float-conversion
        -Wno-switch-enum
        -Wno-shadow
        -Wno-unused-parameter
    )
elseif(MSVC)
    add_compile_options(
        /permissive-
        /Zc:rvalueCast
        /W4
        /FC
        /wd4244
        /wd4100
        /wd4456
    )
endif()

project(END
    VERSION 0.1.0
    LANGUAGES C CXX
    DESCRIPTION "END Audio Plugin"
)

set(JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../JUCE")

find_package(Git REQUIRED)

set(PATCHES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../jam/patch")

set(END_JUCE_PATCHES
    "${PATCHES_PATH}/juce-cached-image-factory-hook.patch"
    "${PATCHES_PATH}/juce-direct2d-helpers-visibility-hook.patch"
    "${PATCHES_PATH}/juce-image-subsection-hook.patch"
    "${PATCHES_PATH}/juce-vulkan-engine-hook.patch"
    "${PATCHES_PATH}/juce-paint-update-rect-hook.patch"
)

if(WIN32)
    set(END_JUCE_TEMP_DIR "$ENV{TEMP}")
elseif(DEFINED ENV{TMPDIR})
    set(END_JUCE_TEMP_DIR "$ENV{TMPDIR}")
else()
    set(END_JUCE_TEMP_DIR "/tmp")
endif()

set(END_PATCHED_JUCE_PATH "${END_JUCE_TEMP_DIR}/juce-patched")
set(END_JUCE_PATCH_STAMP_FILE "${END_PATCHED_JUCE_PATH}/.patch-stamp")

set(END_JUCE_PATCH_SET_CONCAT "")
foreach(END_JUCE_PATCH ${END_JUCE_PATCHES})
    file(READ "${END_JUCE_PATCH}" END_JUCE_PATCH_CONTENT)
    string(APPEND END_JUCE_PATCH_SET_CONCAT "${END_JUCE_PATCH_CONTENT}")
endforeach()

set(END_JUCE_PATCH_SET_CONCAT_FILE "${CMAKE_BINARY_DIR}/end_juce_patch_set_concat.tmp")
file(WRITE "${END_JUCE_PATCH_SET_CONCAT_FILE}" "${END_JUCE_PATCH_SET_CONCAT}")
file(SHA256 "${END_JUCE_PATCH_SET_CONCAT_FILE}" END_JUCE_PATCH_SET_HASH)
file(REMOVE "${END_JUCE_PATCH_SET_CONCAT_FILE}")

set(END_JUCE_PATCH_STAMP_CURRENT "")
if(EXISTS "${END_JUCE_PATCH_STAMP_FILE}")
    file(READ "${END_JUCE_PATCH_STAMP_FILE}" END_JUCE_PATCH_STAMP_CURRENT)
    string(STRIP "${END_JUCE_PATCH_STAMP_CURRENT}" END_JUCE_PATCH_STAMP_CURRENT)
endif()

if(NOT (EXISTS "${END_PATCHED_JUCE_PATH}" AND END_JUCE_PATCH_STAMP_CURRENT STREQUAL END_JUCE_PATCH_SET_HASH))
    if(EXISTS "${END_PATCHED_JUCE_PATH}")
        file(REMOVE_RECURSE "${END_PATCHED_JUCE_PATH}")
    endif()

    file(COPY "${JUCE_PATH}/"
        DESTINATION "${END_PATCHED_JUCE_PATH}"
        PATTERN ".git" EXCLUDE
        PATTERN "docs" EXCLUDE
        PATTERN "DOCS.html" EXCLUDE
    )

    foreach(END_JUCE_PATCH ${END_JUCE_PATCHES})
        get_filename_component(END_JUCE_PATCH_NAME "${END_JUCE_PATCH}" NAME)

        execute_process(
            COMMAND "${GIT_EXECUTABLE}" apply --check --ignore-whitespace "${END_JUCE_PATCH}"
            WORKING_DIRECTORY "${END_PATCHED_JUCE_PATH}"
            RESULT_VARIABLE END_JUCE_PATCH_CHECK_RESULT
            ERROR_VARIABLE END_JUCE_PATCH_CHECK_ERROR
        )
        if(NOT END_JUCE_PATCH_CHECK_RESULT EQUAL 0)
            message(FATAL_ERROR "JUCE patch failed --check: ${END_JUCE_PATCH_NAME}\n${END_JUCE_PATCH_CHECK_ERROR}")
        endif()

        execute_process(
            COMMAND "${GIT_EXECUTABLE}" apply --ignore-whitespace "${END_JUCE_PATCH}"
            WORKING_DIRECTORY "${END_PATCHED_JUCE_PATH}"
            RESULT_VARIABLE END_JUCE_PATCH_RESULT
            ERROR_VARIABLE END_JUCE_PATCH_ERROR
        )
        if(NOT END_JUCE_PATCH_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to apply JUCE patch: ${END_JUCE_PATCH_NAME}\n${END_JUCE_PATCH_ERROR}")
        endif()
    endforeach()

    file(WRITE "${END_JUCE_PATCH_STAMP_FILE}" "${END_JUCE_PATCH_SET_HASH}")
endif()

set(JUCE_PATH "${END_PATCHED_JUCE_PATH}")

add_subdirectory("${JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE")

set(VULKAN_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../Vulkan")

juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_core")
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_debug")
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_data_structures")
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_dsp")
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_gui")
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_graphics")
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_animation")
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_freetype")
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_vulkan")
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_style")
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_markdown")
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_web")
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_terminal")
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_plugin_bootstrap")

juce_add_plugin(END
    COMPANY_NAME "JRENG"
    COMPANY_COPYRIGHT "© 2025. JRENG. All rights reserved."
    COMPANY_WEBSITE "https://jrengmusic.com"
    COMPANY_EMAIL "info@jrengmusic.com"

    PLUGIN_MANUFACTURER_CODE JRNG
    PLUGIN_CODE END.
    BUNDLE_ID "com.jreng.END"

    FORMATS Standalone VST3 AU
    VST3_CATEGORIES Fx

    PRODUCT_NAME "END"
    VERSION 0.1.0

    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE

    COPY_PLUGIN_AFTER_BUILD TRUE
)

juce_generate_juce_header(END)

target_sources(END PRIVATE
    Source/ENDProcessor.h
    Source/ENDProcessor.cpp
    Source/ENDView.h
    Source/ENDView.cpp
)

target_compile_definitions(END PUBLIC
    DONT_SET_USING_JUCE_NAMESPACE=1
    JAM_USING_OVERSAMPLING=0
    JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DONT_DECLARE_PROJECTINFO=1
    $<$<CONFIG:Debug>:DEBUG=1>
    $<$<CONFIG:Release>:NDEBUG=1>
)

target_include_directories(END PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Source"
)

target_include_directories(END PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/generated"
)

target_link_libraries(END
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
    target_include_directories(END BEFORE PRIVATE
        "${VULKAN_PATH}/macOS/include")
endif()

if(WIN32)
    target_include_directories(END BEFORE PRIVATE
        "${VULKAN_PATH}/Include")

    target_compile_definitions(END PRIVATE
        WIN32_LEAN_AND_MEAN=1
        NOMINMAX=1
        _WIN32_IE=0x0A00
        JUCE_CORE_INCLUDE_NATIVE_HEADERS=1
        JUCE_GRAPHICS_INCLUDE_DIRECT2D_HELPERS=1
        JUCE_CORE_INCLUDE_COM_SMART_PTR=1)
endif()

target_include_directories(END PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_freetype/freetype/include")

if(MSVC)
    set_source_files_properties(
        "${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_freetype/jam_freetype.cpp"
        PROPERTIES COMPILE_FLAGS "/wd4005 /wd4804"
    )
else()
    set_source_files_properties(
        "${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_freetype/jam_freetype.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_freetype/jam_freetype.mm"
        PROPERTIES COMPILE_FLAGS "-Wno-macro-redefined"
    )
endif()

target_include_directories(END PRIVATE
    "${JUCE_PATH}/modules/juce_graphics/fonts/harfbuzz")

target_compile_definitions(END PRIVATE VMA_VULKAN_VERSION=1002000)

if(APPLE)
    add_library(moltenVK STATIC IMPORTED GLOBAL)
    set_target_properties(moltenVK PROPERTIES
        IMPORTED_LOCATION
            "${VULKAN_PATH}/macOS/lib/MoltenVK.xcframework/macos-arm64_x86_64/libMoltenVK.a"
        INTERFACE_INCLUDE_DIRECTORIES
            "${VULKAN_PATH}/macOS/include"
    )
    target_link_libraries(END PRIVATE moltenVK)
    target_link_libraries(END PRIVATE
        "-framework IOKit" "-framework IOSurface")
endif()

if(WIN32)
    find_library(END_VULKAN_LOADER_LIB vulkan-1
        PATHS "${VULKAN_PATH}/Lib"
        NO_DEFAULT_PATH
        REQUIRED)
    target_link_libraries(END PRIVATE "${END_VULKAN_LOADER_LIB}")
endif()

if(NOT MSVC)
    set_source_files_properties(
        "${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_vulkan/jam_vulkan.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../jam/jam_vulkan/jam_vulkan.mm"
        PROPERTIES COMPILE_FLAGS "-Wno-nullability-completeness -Wno-nullability-extension -fno-strict-aliasing"
    )
endif()

if(APPLE)
    set(END_VULKAN_GLSLC "${VULKAN_PATH}/macOS/bin/glslc")
elseif(WIN32)
    set(END_VULKAN_GLSLC "${VULKAN_PATH}/Bin/glslc.exe")
endif()

set(END_VULKAN_SHADER_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../jam/resources/shaders")

file(GLOB_RECURSE END_VULKAN_SHADER_SRCS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/../jam/resources/shaders/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/../jam/resources/shaders/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/../jam/resources/shaders/*.comp"
)

foreach(END_SHADER_SRC ${END_VULKAN_SHADER_SRCS})
    get_filename_component(END_SHADER_NAME "${END_SHADER_SRC}" NAME)
    set(END_SHADER_SPV "${CMAKE_CURRENT_SOURCE_DIR}/../jam/resources/spv/${END_SHADER_NAME}.spv")
    set(END_SHADER_DEPFILE "${END_SHADER_SPV}.d")
    add_custom_command(
        OUTPUT  "${END_SHADER_SPV}"
        COMMAND "${END_VULKAN_GLSLC}"
                -I "${END_VULKAN_SHADER_INCLUDE_DIR}"
                -MD -MF "${END_SHADER_DEPFILE}"
                "${END_SHADER_SRC}" -o "${END_SHADER_SPV}"
        DEPENDS "${END_SHADER_SRC}"
        DEPFILE "${END_SHADER_DEPFILE}"
        COMMENT "glslc: ${END_SHADER_NAME} -> spv"
        VERBATIM
    )
endforeach()

file(GLOB END_VULKAN_ALL_SPV_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/../jam/resources/spv/*.spv"
)

if(END_VULKAN_ALL_SPV_FILES)
    juce_add_binary_data(jam_vulkan_ShaderData
        NAMESPACE   "jam"
        HEADER_NAME "JamVulkanShaderData.h"
        SOURCES     ${END_VULKAN_ALL_SPV_FILES}
    )
    target_link_libraries(END PRIVATE jam_vulkan_ShaderData)
endif()

file(GLOB END_JAM_GRAPHICS_FONT_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/../jam/resources/fonts/*.ttf"
)

if(END_JAM_GRAPHICS_FONT_FILES)
    juce_add_binary_data(jam_graphics_FontData
        NAMESPACE   "jam::fonts"
        HEADER_NAME "JamFontsBinaryData.h"
        SOURCES     ${END_JAM_GRAPHICS_FONT_FILES}
    )
    target_link_libraries(END PRIVATE jam_graphics_FontData)
endif()

target_compile_options(END PRIVATE
    $<$<OR:$<PLATFORM_ID:Darwin>,$<PLATFORM_ID:Linux>>:-Wno-float-equal>
    $<$<OR:$<PLATFORM_ID:Darwin>,$<PLATFORM_ID:Linux>>:-Wno-sign-conversion>
    $<$<OR:$<PLATFORM_ID:Darwin>,$<PLATFORM_ID:Linux>>:-Wno-implicit-float-conversion>
    $<$<OR:$<PLATFORM_ID:Darwin>,$<PLATFORM_ID:Linux>>:-Wno-switch-enum>
    $<$<OR:$<PLATFORM_ID:Darwin>,$<PLATFORM_ID:Linux>>:-Wno-shadow>
    $<$<OR:$<PLATFORM_ID:Darwin>,$<PLATFORM_ID:Linux>>:-Wno-unused-parameter>
)

if(MSVC)
    target_compile_options(END PRIVATE
        /permissive-
        /Zc:rvalueCast
        /W4
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
endif()

foreach(END_FORMAT Standalone VST3 AU)
    if(TARGET END_${END_FORMAT})
        target_compile_options(END_${END_FORMAT} PRIVATE
            $<$<OR:$<PLATFORM_ID:Darwin>,$<PLATFORM_ID:Linux>>:-Wno-float-equal>
            $<$<OR:$<PLATFORM_ID:Darwin>,$<PLATFORM_ID:Linux>>:-Wno-sign-conversion>
            $<$<OR:$<PLATFORM_ID:Darwin>,$<PLATFORM_ID:Linux>>:-Wno-implicit-float-conversion>
            $<$<OR:$<PLATFORM_ID:Darwin>,$<PLATFORM_ID:Linux>>:-Wno-switch-enum>
            $<$<OR:$<PLATFORM_ID:Darwin>,$<PLATFORM_ID:Linux>>:-Wno-shadow>
            $<$<OR:$<PLATFORM_ID:Darwin>,$<PLATFORM_ID:Linux>>:-Wno-unused-parameter>
        )
        if(MSVC)
            target_compile_options(END_${END_FORMAT} PRIVATE
                /permissive-
                /Zc:rvalueCast
                /W4
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
        endif()
    endif()
endforeach()

if(WIN32)
    target_link_libraries(END PRIVATE
        dwmapi.lib
        user32.lib
    )
endif()

file(GLOB END_BINARY_DATA_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/layout/*.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/layout/*.css"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/layout/*.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/../jam/resources/fonts/*.ttf"
)

juce_add_binary_data(END_BinaryData
    NAMESPACE "BinaryData"
    SOURCES ${END_BINARY_DATA_FILES}
            ${END_VULKAN_ALL_SPV_FILES}
)

target_link_libraries(END PRIVATE END_BinaryData)
