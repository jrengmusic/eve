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

    extern const juce::String viewLayout;                ///< Virgin settings, seeded to eve.md.
    extern const juce::String settingsLayout;            ///< Settings dialog layout.
    extern const juce::String parametersLayout;          ///< Parameter descriptor tables.
    extern const juce::String profileReport;             ///< Profile report, written on quit by a profile build.
    extern const juce::String markdownSettingsDirectory; ///< User settings directory, relative to home.
    extern const juce::String markdownSettingsFileName;  ///< User settings file name on disk.
    extern const juce::String standaloneSettingsFileName;///< Standalone plugin state file name on disk.
    extern const juce::String upNormal;                  ///< Up-button normal icon.
    extern const juce::String upOver;                    ///< Up-button over icon.
    extern const juce::String upDown;                    ///< Up-button down icon.
    extern const juce::String downNormal;                ///< Down-button normal icon.
    extern const juce::String downOver;                  ///< Down-button over icon.
    extern const juce::String downDown;                  ///< Down-button down icon.
    extern const juce::String shellIntegrationDirectory; ///< Shell hook folder, relative to the settings folder.
    extern const juce::String zshEnvironment;            ///< zsh .zshenv that loads the EVE hooks.
    extern const juce::String zshIntegration;            ///< zsh EVE hooks.
    extern const juce::String bashIntegration;           ///< bash EVE hooks.
    extern const juce::String powershellIntegration;     ///< PowerShell EVE hooks.

/**______________________________END OF NAMESPACE______________________________*/
}// namespace files
