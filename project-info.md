# EVE Metadata

## baseline

```
@brief Toolchain baseline — cmake floor, language standard, platform targets.

One row; each column fills one slot of the generated TOOLCHAIN section.
```

+----------------+-------------+------------------+----------------+-----------------------------------------+
| minimumVersion | cxxStandard | deploymentTarget | architectures  | msvcRuntime                             |
+================+=============+==================+================+=========================================+
| 4.2.0          | 17          | `11.0`           | `x86_64;arm64` | `MultiThreaded$<$<CONFIG:Debug>:Debug>` |
+----------------+-------------+------------------+----------------+-----------------------------------------+

## clang flags

```
@brief Clang warning suppressions — one flag per row, applied to every target.
```

+--------------------------------+
| entry                          |
+================================+
| -Wno-float-equal               |
| -Wno-sign-conversion           |
| -Wno-implicit-float-conversion |
| -Wno-switch-enum               |
| -Wno-shadow                    |
| -Wno-unused-parameter          |
+--------------------------------+

## msvc flags

```
@brief MSVC compiler flags and warning suppressions — one flag per row.
```

+----------------+
| entry          |
+================+
| /permissive-   |
| /Zc:rvalueCast |
| /W4            |
| /FC            |
| /wd4244        |
| /wd4100        |
| /wd4456        |
| /wd4267        |
| /wd4996        |
| /wd4611        |
| /wd4065        |
| /wd4324        |
| /wd4505        |
| /wd4459        |
| /wd4189        |
| /wd4200        |
| /wd4702        |
+----------------+

## project

```
@brief The cmake project declaration — name, version, description.
```

+------+---------+--------------------+
| name | version | description        |
+======+=========+====================+
| EVE  | 0.1.0   | `EVE Audio Plugin` |
+------+---------+--------------------+

## paths

```
@brief Sibling framework and SDK roots, resolved against the project directory.
```

+----------------------------------------+------------------------------------+------------------------------------------+
| jucePath                               | jamPath                            | vulkanPath                               |
+========================================+====================================+==========================================+
| ${CMAKE_CURRENT_SOURCE_DIR}/../../JUCE | ${CMAKE_CURRENT_SOURCE_DIR}/../jam | ${CMAKE_CURRENT_SOURCE_DIR}/../../Vulkan |
+----------------------------------------+------------------------------------+------------------------------------------+

## juce

```
@brief The required JUCE version — gated exact after add_subdirectory.
```

+---------+
| version |
+=========+
| 8.0.14  |
+---------+

## patches

```
@brief JUCE patch set — file names under jam/patch, applied in row order.
```

+---------------------------------------------+
| patch                                       |
+=============================================+
| juce-cached-image-factory-hook.patch        |
| juce-direct2d-helpers-visibility-hook.patch |
| juce-image-subsection-hook.patch            |
| juce-vulkan-engine-hook.patch               |
| juce-paint-update-rect-hook.patch           |
+---------------------------------------------+

## modules

```
@brief JAM modules added with juce_add_module, in row order.
```

+----------------------+
| module               |
+======================+
| jam_core             |
| jam_debug            |
| jam_data_structures  |
| jam_dsp              |
| jam_gui              |
| jam_graphics         |
| jam_animation        |
| jam_freetype         |
| jam_vulkan           |
| jam_style            |
| jam_markdown         |
| jam_web              |
| jam_terminal         |
| jam_plugin_bootstrap |
+----------------------+

## plugin

```
@brief The juce_add_plugin declaration — Projucer-parity plugin characteristics.

PRODUCT_NAME defaults to the target name and VERSION to the project version;
neither is restated here.
```

+-------------+---------------------------------------+--------------------------+-----------------------+------------------+------------+-----------------+--------------------+----------------+---------+----------------+-----------------+--------------+--------------------------+----------------------+
| companyName | companyCopyright                      | companyWebsite           | companyEmail          | manufacturerCode | pluginCode | bundleId        | formats            | vst3Categories | isSynth | needsMidiInput | needsMidiOutput | isMidiEffect | editorWantsKeyboardFocus | copyPluginAfterBuild |
+=============+=======================================+==========================+=======================+==================+============+=================+====================+================+=========+================+=================+==============+==========================+======================+
| `JRENG`     | "© 2025. JRENG. All rights reserved." | `https://jrengmusic.com` | `info@jrengmusic.com` | JRNG             | EVE.       | `com.jreng.EVE` | Standalone VST3 AU | Fx             | FALSE   | FALSE          | FALSE           | FALSE        | FALSE                    | TRUE                 |
+-------------+---------------------------------------+--------------------------+-----------------------+------------------+------------+-----------------+--------------------+----------------+---------+----------------+-----------------+--------------+--------------------------+----------------------+

## sources

```
@brief The target's own source files, one per row.
```

+-------------------------+
| entry                   |
+=========================+
| Source/EVEProcessor.h   |
| Source/EVEProcessor.cpp |
| Source/EVEView.h        |
| Source/EVEView.cpp      |
+-------------------------+

## defines

```
@brief Compile definitions — one per row; platform and configuration rows
route through generator expressions.
```

+----------------------------------------------------------------------+
| entry                                                                |
+======================================================================+
| DONT_SET_USING_JUCE_NAMESPACE=1                                      |
| JAM_USING_OVERSAMPLING=0                                             |
| JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1                              |
| JUCE_VST3_CAN_REPLACE_VST2=0                                         |
| JUCE_STRICT_REFCOUNTEDPOINTER=1                                      |
| JUCE_WEB_BROWSER=0                                                   |
| JUCE_USE_CURL=0                                                      |
| JUCE_DONT_DECLARE_PROJECTINFO=1                                      |
| VMA_VULKAN_VERSION=1002000                                           |
| `$<$<CONFIG:Debug>:DEBUG=1>`                                         |
| `$<$<CONFIG:Release>:NDEBUG=1>`                                      |
| `$<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN=1>`                    |
| `$<$<PLATFORM_ID:Windows>:NOMINMAX=1>`                               |
| `$<$<PLATFORM_ID:Windows>:_WIN32_IE=0x0A00>`                         |
| `$<$<PLATFORM_ID:Windows>:JUCE_CORE_INCLUDE_NATIVE_HEADERS=1>`       |
| `$<$<PLATFORM_ID:Windows>:JUCE_GRAPHICS_INCLUDE_DIRECT2D_HELPERS=1>` |
| `$<$<PLATFORM_ID:Windows>:JUCE_CORE_INCLUDE_COM_SMART_PTR=1>`        |
+----------------------------------------------------------------------+

## includes

```
@brief Header search roots — project, generated, and vendored.
```

+----------------------------------------------------------+
| entry                                                    |
+==========================================================+
| `${CMAKE_CURRENT_SOURCE_DIR}/Source`                     |
| `${CMAKE_CURRENT_SOURCE_DIR}/Source/generated`           |
| `${CAST_JAM_PATH}/jam_freetype/freetype/include`         |
| `${CAST_JUCE_PATH}/modules/juce_graphics/fonts/harfbuzz` |
+----------------------------------------------------------+

## libraries

```
@brief Private link libraries — JUCE utilities and every linked JAM module.
```

+--------------------------+
| entry                    |
+==========================+
| juce::juce_audio_utils   |
| juce::juce_audio_devices |
| jam_core                 |
| jam_debug                |
| jam_data_structures      |
| jam_dsp                  |
| jam_gui                  |
| jam_graphics             |
| jam_animation            |
| jam_freetype             |
| jam_vulkan               |
| jam_style                |
| jam_markdown             |
| jam_web                  |
| jam_plugin_bootstrap     |
+--------------------------+

## layout globs

```
@brief Binary-data glob patterns for the project's own embedded resources.
```

+---------------------------------------------------+
| entry                                             |
+===================================================+
| `${CMAKE_CURRENT_SOURCE_DIR}/Source/layout/*.md`  |
| `${CMAKE_CURRENT_SOURCE_DIR}/Source/layout/*.css` |
| `${CMAKE_CURRENT_SOURCE_DIR}/Source/layout/*.xml` |
| `${CAST_JAM_PATH}/resources/fonts/*.ttf`          |
+---------------------------------------------------+

## project info

```
@brief Project metadata — the ProjectInfo namespace, generated.

Every field is a complete literal; nothing downstream derives, concatenates, or restates a value.
```

+-------------------+------------------+--------+--------------------------+-----------+--------------------------------------------------+
| type              | name             | format | value                    | format    | comment                                          |
+===================+==================+========+==========================+===========+==================================================+
| const char* const | projectName      |        | EVE                      | toLiteral | Product name.                                    |
| const char* const | companyName      |        | JRENG                    | toLiteral | Company name.                                    |
| const char* const | legalCompanyName |        | PT JRENG Teknika         | toLiteral | Full legal company name.                         |
| const char* const | versionString    |        | 0.1.0                    | toLiteral | Product version string.                          |
| int               | versionNumber    |        | 0x100                    |           | Product version, JUCE hex encoding.              |
| const char* const | productWebsite   |        | `https://jrengmusic.com` |           | Product website URL.                             |
| const char* const | presetExtension  |        | endp                     | toLiteral | Preset file extension, without the leading dot.  |
| const char* const | presetDefault    |        | INIT                     | toLiteral | Default init preset name, without the extension. |
+-------------------+------------------+--------+--------------------------+-----------+--------------------------------------------------+
