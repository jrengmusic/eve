/*******************************************************************************
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
********************************************************************************/

#pragma once

namespace files
{
/*_____________________________________________________________________________*/

/**
 * @brief Product layout file names.
 *
 * Each constant is the literal file name of an embedded layout resource,
 * resolved against the binary-data / asset search path at load time.
 */

    extern const juce::String viewLayout;      ///< Virgin config: UI size and configuration, seeded to eve.md.
    extern const juce::String panelLayout;     ///< Top panel: settings and about buttons.
    extern const juce::String settingsLayout;  ///< Settings dialog layout.
    extern const juce::String aboutLayout;     ///< About dialog layout.
    extern const juce::String parametersLayout;///< Parameter descriptor tables.
    extern const juce::String configDirectory; ///< User config directory, relative to home.
    extern const juce::String defaultConfig;   ///< User config file name on disk.
    extern const juce::String settingsNormal;  ///< Settings-button normal icon.
    extern const juce::String settingsOver;    ///< Settings-button over icon.
    extern const juce::String settingsDown;    ///< Settings-button down icon.
    extern const juce::String upNormal;        ///< Up-button normal icon.
    extern const juce::String upOver;          ///< Up-button over icon.
    extern const juce::String upDown;          ///< Up-button down icon.
    extern const juce::String downNormal;      ///< Down-button normal icon.
    extern const juce::String downOver;        ///< Down-button over icon.
    extern const juce::String downDown;        ///< Down-button down icon.

/**______________________________END OF NAMESPACE______________________________*/
}// namespace files
