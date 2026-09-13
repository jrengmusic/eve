cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(ansi_fixpoint
    VERSION "0.0.1"
    LANGUAGES C CXX
    DESCRIPTION "AnsiDocument byte-fixpoint harness"
)

set(ANSI_FIXPOINT_JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../../JUCE")
set(ANSI_FIXPOINT_JAM_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../jam")

add_subdirectory("${ANSI_FIXPOINT_JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE")

juce_add_module("${ANSI_FIXPOINT_JAM_PATH}/jam_core")
juce_add_module("${ANSI_FIXPOINT_JAM_PATH}/jam_data_structures")
juce_add_module("${ANSI_FIXPOINT_JAM_PATH}/jam_graphics")
juce_add_module("${ANSI_FIXPOINT_JAM_PATH}/jam_terminal")

juce_add_console_app(ansi_fixpoint
    PRODUCT_NAME "ansi_fixpoint"
)

juce_generate_juce_header(ansi_fixpoint)

target_sources(ansi_fixpoint PRIVATE main.cpp)

target_include_directories(ansi_fixpoint PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${ANSI_FIXPOINT_JUCE_PATH}/modules/juce_graphics/fonts/harfbuzz"
)

target_compile_definitions(ansi_fixpoint PRIVATE
    DONT_SET_USING_JUCE_NAMESPACE=1
    JUCE_DONT_DECLARE_PROJECTINFO=1
    ANSI_FIXPOINT_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
)

target_link_libraries(ansi_fixpoint
    PRIVATE
        jam_core
        jam_data_structures
        jam_graphics
        jam_terminal
    PUBLIC
        juce::juce_recommended_config_flags
)
