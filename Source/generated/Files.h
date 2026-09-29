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

    extern const juce::String viewLayout;      ///< Editor geometry and UI size.
    extern const juce::String panelLayout;     ///< Status bar layout (bottom ViewPanel row).
    extern const juce::String configDirectory; ///< User config directory, relative to home.
    extern const juce::String defaultConfig;   ///< User config document seeded when missing.
    extern const juce::String parametersLayout;///< Parameter descriptor tables.

/**______________________________END OF NAMESPACE______________________________*/
}// namespace files
