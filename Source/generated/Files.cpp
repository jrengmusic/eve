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

#include <JuceHeader.h>
#include "Files.h"

namespace files
{
/*_____________________________________________________________________________*/

/**
 * @brief Product layout file names.
 *
 * Each constant is the literal file name of an embedded layout resource,
 * resolved against the binary-data / asset search path at load time.
 */

    const juce::String viewLayout       { juce::String::fromUTF8 ("ViewLayout.md") };
    const juce::String panelLayout      { juce::String::fromUTF8 ("PanelLayout.html") };
    const juce::String settingsLayout   { juce::String::fromUTF8 ("SettingsLayout.html") };
    const juce::String aboutLayout      { juce::String::fromUTF8 ("AboutLayout.html") };
    const juce::String parametersLayout { juce::String::fromUTF8 ("parameters.md") };
    const juce::String configDirectory  { juce::String::fromUTF8 (".config/end") };
    const juce::String defaultConfig    { juce::String::fromUTF8 ("eve.md") };
    const juce::String settingsNormal   { juce::String::fromUTF8 ("settings_normal.svg") };
    const juce::String settingsOver     { juce::String::fromUTF8 ("settings_over.svg") };
    const juce::String settingsDown     { juce::String::fromUTF8 ("settings_down.svg") };
    const juce::String upNormal         { juce::String::fromUTF8 ("up_normal.svg") };
    const juce::String upOver           { juce::String::fromUTF8 ("up_over.svg") };
    const juce::String upDown           { juce::String::fromUTF8 ("up_down.svg") };
    const juce::String downNormal       { juce::String::fromUTF8 ("down_normal.svg") };
    const juce::String downOver         { juce::String::fromUTF8 ("down_over.svg") };
    const juce::String downDown         { juce::String::fromUTF8 ("down_down.svg") };

/**______________________________END OF NAMESPACE______________________________*/
}// namespace files
