# EVE Metadata

## index

+--------------+------------------------------------+
| alias        | symbol                             |
+==============+====================================+
| @char        | const char* const                  |
| @user-module | ${CMAKE_CURRENT_SOURCE_DIR}/../jam |
| @patch       | ${CAST_USER_MODULE_PATH}/patch     |
+--------------+------------------------------------+

## project info

```
@brief Project metadata — the ProjectInfo namespace, generated.

Every field is a complete literal; nothing downstream derives, concatenates, or restates a value.
```

+------------------+-------+--------------------------------------------------+-----------+--------------------------------------------------+
| name             | type  | value                                            | format    | comment                                          |
+==================+=======+==================================================+===========+==================================================+
| projectName      | @char | EVE                                              | toLiteral | Product name.                                    |
| companyName      | @char | JRENG                                            | toLiteral | Company name.                                    |
| legalCompanyName | @char | Jubilant Research of Eclectic Novelty Generation | toLiteral | Full legal company name.                         |
| versionString    | @char | 0.1.0                                            | toLiteral | Product version string.                          |
| versionNumber    | int   | 0x100                                            |           | Product version, JUCE hex encoding.              |
| productWebsite   | @char | `https://jrengmusic.com`                         |           | Product website URL.                             |
| companyCopyright | @char | (c) 2025 JRENG. All rights reserved.             | toLiteral | Company copyright line.                          |
| companyEmail     | @char | info@jrengmusic.com                              | toLiteral | Company contact email.                           |
| presetExtension  | @char | endp                                             | toLiteral | Preset file extension, without the leading dot.  |
| presetDefault    | @char | INIT                                             | toLiteral | Default init preset name, without the extension. |
| versionHint      | int   | 1                                                |           | juce::ParameterID version hint.                  |
+------------------+-------+--------------------------------------------------+-----------+--------------------------------------------------+

## cmake

+-----------------------------+-----------------------------------------------+---------------------------------------------+
| key                         | value                                         | comment                                     |
+=============================+===============================================+=============================================+
| minimumVersion              | 4.2.0                                         | CMake minimum version                       |
| cxxStandard                 | 17                                            | C++ language standard                       |
| juceTargetFunction          | juce_add_plugin                               | JUCE target-creation function               |
| deploymentTarget            | `11.0`                                        | Minimum macOS deployment target             |
| msvcRuntime                 | `MultiThreaded$<$<CONFIG:Debug>:Debug>`       | MSVC runtime library selection              |
| description                 | `EVE Audio Plugin`                            | Project description                         |
| jucePath                    | ${CMAKE_CURRENT_SOURCE_DIR}/../../JUCE        | JUCE root                                   |
| userModulePath              | @user-module                                  | User module root                            |
| vulkanPath                  | ${CMAKE_CURRENT_SOURCE_DIR}/../../Vulkan      | Vulkan SDK root                             |
| juceVersion                 | 8.0.14                                        | Required JUCE version, exact                |
| manufacturerCode            | JRNG                                          | Four-character manufacturer code            |
| pluginCode                  | EVE.                                          | Four-character plugin code                  |
| bundleIdentifier            | `com.jreng.EVE`                               | Bundle identifier                           |
| targetName                  | EVE                                           | CMake target name                           |
| productName                 | EVE                                           | JUCE PRODUCT_NAME                           |
| vst3Categories              | Fx                                            | VST3 plugin categories                      |
| isSynth                     | FALSE                                         | Plugin is a synthesizer                     |
| needsMidiInput              | FALSE                                         | Plugin accepts MIDI                         |
| needsMidiOutput             | FALSE                                         | Plugin produces MIDI                        |
| isMidiEffect                | FALSE                                         | Plugin processes MIDI only                  |
| editorWantsKeyboardFocus    | FALSE                                         | Editor requires keyboard focus              |
| copyPluginAfterBuild        | FALSE                                         | JUCE copy step, replaced by CAST post-build |
| interproceduralOptimization | ON                                            | CMAKE_INTERPROCEDURAL_OPTIMIZATION          |
| qaDirectory                 | `$ENV{HOME}/Documents/Poems/dev/___builds___` | QA release archive root                     |
+-----------------------------+-----------------------------------------------+---------------------------------------------+

## toolchain

+----------+---------+----------------------------------------------------------------------------+
| argument | command | flag                                                                       |
+==========+=========+============================================================================+
|          | cast    | ../jam/cast/CAST.md                                                        |
+----------+---------+----------------------------------------------------------------------------+
|          | cmake   | -S . -B Builds/Release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCAST_SIGN=ON  |
+----------+---------+----------------------------------------------------------------------------+
|          | ninja   | -C Builds/Release                                                          |
+----------+---------+----------------------------------------------------------------------------+
| debug    | cast    | ../jam/cast/CAST.md                                                        |
+----------+---------+----------------------------------------------------------------------------+
| debug    | cmake   | -S . -B Builds/Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug                     |
+----------+---------+----------------------------------------------------------------------------+
| debug    | ninja   | -C Builds/Debug                                                            |
+----------+---------+----------------------------------------------------------------------------+
| no-sign  | cast    | ../jam/cast/CAST.md                                                        |
+----------+---------+----------------------------------------------------------------------------+
| no-sign  | cmake   | -S . -B Builds/Release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCAST_SIGN=OFF |
+----------+---------+----------------------------------------------------------------------------+
| no-sign  | ninja   | -C Builds/Release                                                          |
+----------+---------+----------------------------------------------------------------------------+

## signing

+------------------+--------------------------------------------------------------+--------------------------------------+
| key              | value                                                        | comment                              |
+==================+==============================================================+======================================+
| identity         | Developer ID Application: Bayu Ardianto \\\\(9BDSN9TDX3\\\\) | Code signing identity                |
+------------------+--------------------------------------------------------------+--------------------------------------+
| entitlementsPath | entitlements.plist                                           | Entitlements file, project root      |
+------------------+--------------------------------------------------------------+--------------------------------------+
| notaryProfile    | notary                                                       | Keychain notarization profile        |
+------------------+--------------------------------------------------------------+--------------------------------------+
| edenAccount      | bayu@jrengmusic.com                                          | PACE Eden account                    |
+------------------+--------------------------------------------------------------+--------------------------------------+
| wrapConfigGuid   | 4B0468E0-A60A-11F1-BA61-005056928F3B                         | PACE wrap configuration, EVE product |
+------------------+--------------------------------------------------------------+--------------------------------------+

## architecture

+--------+
| value  |
+========+
| x86_64 |
| arm64  |
+--------+

## format

+------------+------------+--------------------------------------------------+---------+
| name       | value      | installDirectory                                 | comment |
+============+============+==================================================+=========+
| standalone | Standalone |                                                  |         |
| vst3       | VST3       | $ENV{HOME}/Library/Audio/Plug-Ins/VST3           |         |
| au         | AU         | $ENV{HOME}/Library/Audio/Plug-Ins/Components     |         |
| aax        | AAX        | /Library/Application Support/Avid/Audio/Plug-Ins |         |
+------------+------------+--------------------------------------------------+---------+

## release

+-------------------------+--------------------------------+----------------+--------+---------------------------------------------------------+
| name                    | mac                            | win            | stage  | comment                                                 |
+=========================+================================+================+========+=========================================================+
| shadow                  | -Wno-shadow                    | /wd4456        |        | Lambda captures / declarations may shadow intentionally |
+-------------------------+--------------------------------+----------------+--------+---------------------------------------------------------+
| shadowGlobal            |                                | /wd4459        |        | Declaration hides global declaration                    |
+-------------------------+--------------------------------+----------------+--------+---------------------------------------------------------+
| unusedParameter         | -Wno-unused-parameter          | /wd4100        |        | Debug/template code may not use all parameters          |
+-------------------------+--------------------------------+----------------+--------+---------------------------------------------------------+
| unusedVariable          |                                | /wd4189        |        | Local variable initialized but not referenced           |
+-------------------------+--------------------------------+----------------+--------+---------------------------------------------------------+
| unusedFunction          |                                | /wd4505        |        | Unreferenced local function removed                     |
+-------------------------+--------------------------------+----------------+--------+---------------------------------------------------------+
| floatEqual              | -Wno-float-equal               |                |        | DSP exact float comparisons for bypass detection        |
+-------------------------+--------------------------------+----------------+--------+---------------------------------------------------------+
| signConversion          | -Wno-sign-conversion           |                |        | Array indexing, safe in this context                    |
+-------------------------+--------------------------------+----------------+--------+---------------------------------------------------------+
| sizeConversion          |                                | /wd4267        |        | size_t conversion, possible loss of data                |
+-------------------------+--------------------------------+----------------+--------+---------------------------------------------------------+
| switchEnum              | -Wno-switch-enum               |                |        | Not every filter type needs every case handled          |
+-------------------------+--------------------------------+----------------+--------+---------------------------------------------------------+
| switchNoCase            |                                | /wd4065        |        | switch with default but no case                         |
+-------------------------+--------------------------------+----------------+--------+---------------------------------------------------------+
| floatToDoubleConversion | -Wno-implicit-float-conversion | /wd4244        |        | DSP double/float conversions                            |
+-------------------------+--------------------------------+----------------+--------+---------------------------------------------------------+
| deprecated              |                                | /wd4996        |        | Deprecated function or symbol                           |
+-------------------------+--------------------------------+----------------+--------+---------------------------------------------------------+
| setjmpLongjmp           |                                | /wd4611        |        | setjmp/longjmp with C++ objects                         |
+-------------------------+--------------------------------+----------------+--------+---------------------------------------------------------+
| structurePadding        |                                | /wd4324        |        | Structure padded due to alignment specifier             |
+-------------------------+--------------------------------+----------------+--------+---------------------------------------------------------+
| zeroSizedArray          |                                | /wd4200        |        | Zero-sized array in struct/union                        |
+-------------------------+--------------------------------+----------------+--------+---------------------------------------------------------+
| unreachableCode         |                                | /wd4702        |        | Unreachable code                                        |
+-------------------------+--------------------------------+----------------+--------+---------------------------------------------------------+
| permissiveMinus         |                                | /permissive-   |        | Standards conformance mode                              |
+-------------------------+--------------------------------+----------------+--------+---------------------------------------------------------+
| rvalueCast              |                                | /Zc:rvalueCast |        | Standard rvalue cast conformance                        |
+-------------------------+--------------------------------+----------------+--------+---------------------------------------------------------+
| warningLevel4           |                                | /W4            |        | Warning level 4                                         |
+-------------------------+--------------------------------+----------------+--------+---------------------------------------------------------+
| fullPathInPdb           |                                | /FC            |        | Full source path in diagnostics                         |
+-------------------------+--------------------------------+----------------+--------+---------------------------------------------------------+
| optimization            | -O3                            | /O2            |        | Full optimization                                       |
+-------------------------+--------------------------------+----------------+--------+---------------------------------------------------------+
| linkTimeOptimization    | -flto=thin                     | /GL            |        | Link-time optimization codegen                          |
+-------------------------+--------------------------------+----------------+--------+---------------------------------------------------------+
| linkTimeCodegen         |                                | /LTCG          | linker | MSVC whole-program link-time codegen                    |
+-------------------------+--------------------------------+----------------+--------+---------------------------------------------------------+
| deadCodeStripping       | -dead_strip                    | /OPT:REF       | linker | Strip unreferenced functions and data                   |
+-------------------------+--------------------------------+----------------+--------+---------------------------------------------------------+
| identicalCodeFolding    |                                | /OPT:ICF       | linker | Fold identical COMDATs                                  |
+-------------------------+--------------------------------+----------------+--------+---------------------------------------------------------+

## debug

+-------------------------+--------------------------------+----------------+-------+---------------------------------------------------------+
| name                    | mac                            | win            | stage | comment                                                 |
+=========================+================================+================+=======+=========================================================+
| shadow                  | -Wno-shadow                    | /wd4456        |       | Lambda captures / declarations may shadow intentionally |
+-------------------------+--------------------------------+----------------+-------+---------------------------------------------------------+
| shadowGlobal            |                                | /wd4459        |       | Declaration hides global declaration                    |
+-------------------------+--------------------------------+----------------+-------+---------------------------------------------------------+
| unusedParameter         | -Wno-unused-parameter          | /wd4100        |       | Debug/template code may not use all parameters          |
+-------------------------+--------------------------------+----------------+-------+---------------------------------------------------------+
| unusedVariable          |                                | /wd4189        |       | Local variable initialized but not referenced           |
+-------------------------+--------------------------------+----------------+-------+---------------------------------------------------------+
| unusedFunction          |                                | /wd4505        |       | Unreferenced local function removed                     |
+-------------------------+--------------------------------+----------------+-------+---------------------------------------------------------+
| floatEqual              | -Wno-float-equal               |                |       | DSP exact float comparisons for bypass detection        |
+-------------------------+--------------------------------+----------------+-------+---------------------------------------------------------+
| signConversion          | -Wno-sign-conversion           |                |       | Array indexing, safe in this context                    |
+-------------------------+--------------------------------+----------------+-------+---------------------------------------------------------+
| sizeConversion          |                                | /wd4267        |       | size_t conversion, possible loss of data                |
+-------------------------+--------------------------------+----------------+-------+---------------------------------------------------------+
| switchEnum              | -Wno-switch-enum               |                |       | Not every filter type needs every case handled          |
+-------------------------+--------------------------------+----------------+-------+---------------------------------------------------------+
| switchNoCase            |                                | /wd4065        |       | switch with default but no case                         |
+-------------------------+--------------------------------+----------------+-------+---------------------------------------------------------+
| floatToDoubleConversion | -Wno-implicit-float-conversion | /wd4244        |       | DSP double/float conversions                            |
+-------------------------+--------------------------------+----------------+-------+---------------------------------------------------------+
| deprecated              |                                | /wd4996        |       | Deprecated function or symbol                           |
+-------------------------+--------------------------------+----------------+-------+---------------------------------------------------------+
| setjmpLongjmp           |                                | /wd4611        |       | setjmp/longjmp with C++ objects                         |
+-------------------------+--------------------------------+----------------+-------+---------------------------------------------------------+
| structurePadding        |                                | /wd4324        |       | Structure padded due to alignment specifier             |
+-------------------------+--------------------------------+----------------+-------+---------------------------------------------------------+
| zeroSizedArray          |                                | /wd4200        |       | Zero-sized array in struct/union                        |
+-------------------------+--------------------------------+----------------+-------+---------------------------------------------------------+
| unreachableCode         |                                | /wd4702        |       | Unreachable code                                        |
+-------------------------+--------------------------------+----------------+-------+---------------------------------------------------------+
| permissiveMinus         |                                | /permissive-   |       | Standards conformance mode                              |
+-------------------------+--------------------------------+----------------+-------+---------------------------------------------------------+
| rvalueCast              |                                | /Zc:rvalueCast |       | Standard rvalue cast conformance                        |
+-------------------------+--------------------------------+----------------+-------+---------------------------------------------------------+
| warningLevel4           |                                | /W4            |       | Warning level 4                                         |
+-------------------------+--------------------------------+----------------+-------+---------------------------------------------------------+
| fullPathInPdb           |                                | /FC            |       | Full source path in diagnostics                         |
+-------------------------+--------------------------------+----------------+-------+---------------------------------------------------------+
| optimization            | -O0                            | /Od            |       | No optimization                                         |
+-------------------------+--------------------------------+----------------+-------+---------------------------------------------------------+
| debugSymbols            | -g                             | /Zi            |       | Debug symbols                                           |
+-------------------------+--------------------------------+----------------+-------+---------------------------------------------------------+

## patch

+--------+---------------------------------------------+----------------------------------------------+
| root   | name                                        | comment                                      |
+========+=============================================+==============================================+
| @patch | juce-cached-image-factory-hook.patch        | External CachedComponentImage factory        |
| @patch | juce-direct2d-helpers-visibility-hook.patch | Direct2D helpers visibility gate             |
| @patch | juce-image-subsection-hook.patch            | Root image and subsection bounds hooks       |
| @patch | juce-vulkan-engine-hook.patch               | External graphics context factory for Vulkan |
| @patch | juce-paint-update-rect-hook.patch           | OS dirty rectangle before native paint       |
+--------+---------------------------------------------+----------------------------------------------+

## user module

+--------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------+
| root         | name                 | comment                                                                                                                                |
+==============+======================+========================================================================================================================================+
| @user-module | jam_core             | JAM Core                                                                                                                               |
| @user-module | jam_debug            | Debug instruments — console, log, model monitor                                                                                        |
| @user-module | jam_data_structures  | ValueTree management and data model utilities — model, parameters, JSON conversion                                                     |
| @user-module | jam_dsp              | DSP processors — filters, waveshaping, hysteresis modelling, transient control, oversampling, FIR, noise generation, spectrum analysis |
| @user-module | jam_gui              | GUI foundation — Window, Modal, Glass                                                                                                  |
| @user-module | jam_graphics         | Graphics utilities, blur, shadows, colours, fonts, mesh                                                                                |
| @user-module | jam_animation        | Foundation animation classes (Animator, AnimationBase, AnimationScrollingText)                                                         |
| @user-module | jam_freetype         | Vendored FreeType font rasterization library                                                                                           |
| @user-module | jam_vulkan           | Vulkan rendering backend                                                                                                               |
| @user-module | jam_style            | JAM Style — LookAndFeel base + ColourScheme-backed colour registry                                                                     |
| @user-module | jam_markdown         | Clean-room native CommonMark + GFM markdown parsing and rendering                                                                      |
| @user-module | jam_web              | HTML authored-subset and CSS Syntax Level 3 subset tokenizers and parsers                                                              |
| @user-module | jam_plugin_bootstrap | Document-driven plugin bootstrap — view construction, style management, plugin editor base, standalone shell                           |
+--------------+----------------------+----------------------------------------------------------------------------------------------------------------------------------------+

## juce module

+--------------+--------------------+------------------------------------------------------------+
| name         | value              | comment                                                    |
+==============+====================+============================================================+
| audioUtils   | juce_audio_utils   | Classes for audio-related GUI and miscellaneous tasks.     |
+--------------+--------------------+------------------------------------------------------------+
| audioDevices | juce_audio_devices | Classes to play and record from audio and MIDI I/O devices |
+--------------+--------------------+------------------------------------------------------------+

## source

+----------------------+----------------------------+---------+
| name                 | value                      | comment |
+======================+============================+=========+
| processorHeader      | Source/EVEProcessor.h      |         |
| processorSource      | Source/EVEProcessor.cpp    |         |
| audioProcessorHeader | Source/EVEAudioProcessor.h |         |
| viewHeader           | Source/EVEView.h           |         |
| viewSource           | Source/EVEView.cpp         |         |
+----------------------+----------------------------+---------+

## define

+------------------------------------+----------------------------------------------------------------------+------------------------------------------------+
| name                               | value                                                                | comment                                        |
+====================================+======================================================================+================================================+
| dontSetUsingJuceNamespace          | DONT_SET_USING_JUCE_NAMESPACE=1                                      | No using namespace juce in JuceHeader.h        |
| jamUsingOversampling               | JAM_USING_OVERSAMPLING=0                                             | JAM oversampling feature                       |
| juceUseCustomPluginStandaloneApp   | JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1                              | Custom standalone app entry point              |
| juceVst3CanReplaceVst2             | JUCE_VST3_CAN_REPLACE_VST2=0                                         | VST3 loads VST2-compatible state               |
| juceStrictRefCountedPointer        | JUCE_STRICT_REFCOUNTEDPOINTER=1                                      | Strict ReferenceCountedObjectPtr casting       |
| juceWebBrowser                     | JUCE_WEB_BROWSER=0                                                   | WebBrowserComponent support                    |
| juceUseCurl                        | JUCE_USE_CURL=0                                                      | libcurl for http on Linux                      |
| juceDontDeclareProjectInfo         | JUCE_DONT_DECLARE_PROJECTINFO=1                                      | No auto-generated ProjectInfo namespace        |
| vmaVulkanVersion                   | VMA_VULKAN_VERSION=1002000                                           | Vulkan version for Vulkan Memory Allocator     |
| debug                              | `$<$<CONFIG:Debug>:DEBUG=1>`                                         | Debug configuration                            |
| ndebug                             | `$<$<CONFIG:Release>:NDEBUG=1>`                                      | Release configuration                          |
| win32LeanAndMean                   | `$<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN=1>`                    | Exclude rarely used Windows headers            |
| nominmax                           | `$<$<PLATFORM_ID:Windows>:NOMINMAX=1>`                               | No Windows min/max macros                      |
| win32Ie                            | `$<$<PLATFORM_ID:Windows>:_WIN32_IE=0x0A00>`                         | Windows IE platform version                    |
| juceCoreIncludeNativeHeaders       | `$<$<PLATFORM_ID:Windows>:JUCE_CORE_INCLUDE_NATIVE_HEADERS=1>`       | Native platform headers in juce_core           |
| juceGraphicsIncludeDirect2dHelpers | `$<$<PLATFORM_ID:Windows>:JUCE_GRAPHICS_INCLUDE_DIRECT2D_HELPERS=1>` | Direct2D helper visibility, from the jam patch |
| juceCoreIncludeComSmartPtr         | `$<$<PLATFORM_ID:Windows>:JUCE_CORE_INCLUDE_COM_SMART_PTR=1>`        | Windows COM smart pointer helpers              |
+------------------------------------+----------------------------------------------------------------------+------------------------------------------------+

## include

+-----------+----------------------------------------------------------+---------+
| name      | value                                                    | comment |
+===========+==========================================================+=========+
| source    | `${CMAKE_CURRENT_SOURCE_DIR}/Source`                     |         |
| generated | `${CMAKE_CURRENT_SOURCE_DIR}/Source/generated`           |         |
| freetype  | `${CAST_USER_MODULE_PATH}/jam_freetype/freetype/include` |         |
| harfbuzz  | `${CAST_JUCE_PATH}/modules/juce_graphics/fonts/harfbuzz` |         |
+-----------+----------------------------------------------------------+---------+

## layout glob

+----------+----------------------------------------------------+---------+
| name     | value                                              | comment |
+==========+====================================================+=========+
| markdown | `${CMAKE_CURRENT_SOURCE_DIR}/Source/layout/*.md`   |         |
| css      | `${CMAKE_CURRENT_SOURCE_DIR}/Source/layout/*.css`  |         |
| xml      | `${CMAKE_CURRENT_SOURCE_DIR}/Source/layout/*.xml`  |         |
| html     | `${CMAKE_CURRENT_SOURCE_DIR}/Source/layout/*.html` |         |
| svg      | `${CMAKE_CURRENT_SOURCE_DIR}/Source/layout/*.svg`  |         |
| fonts    | `${CAST_USER_MODULE_PATH}/resources/fonts/*.ttf`   |         |
+----------+----------------------------------------------------+---------+
