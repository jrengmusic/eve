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

    const juce::String viewLayout                 { juce::String::fromUTF8 ("ViewLayout.md") };
    const juce::String settingsLayout             { juce::String::fromUTF8 ("SettingsLayout.html") };
    const juce::String parametersLayout           { juce::String::fromUTF8 ("parameters.md") };
    const juce::String modelLayout                { juce::String::fromUTF8 ("model.md") };
    const juce::String markdownSettingsDirectory  { juce::String::fromUTF8 (".config/end") };
    const juce::String markdownSettingsFileName   { juce::String::fromUTF8 ("eve.md") };
    const juce::String standaloneSettingsFileName { juce::String::fromUTF8 ("EVE.settings") };
    const juce::String upNormal                   { juce::String::fromUTF8 ("up_normal.svg") };
    const juce::String upOver                     { juce::String::fromUTF8 ("up_over.svg") };
    const juce::String upDown                     { juce::String::fromUTF8 ("up_down.svg") };
    const juce::String downNormal                 { juce::String::fromUTF8 ("down_normal.svg") };
    const juce::String downOver                   { juce::String::fromUTF8 ("down_over.svg") };
    const juce::String downDown                   { juce::String::fromUTF8 ("down_down.svg") };
    const juce::String shellIntegrationDirectory  { juce::String::fromUTF8 ("shell") };
    const juce::String zshEnvironment             { juce::String::fromUTF8 ("zsh_zshenv.zsh") };
    const juce::String zshIntegration             { juce::String::fromUTF8 ("zsh_integration.zsh") };
    const juce::String bashIntegration            { juce::String::fromUTF8 ("bash_integration.bash") };
    const juce::String powershellIntegration      { juce::String::fromUTF8 ("powershell_integration.ps1") };

/**______________________________END OF NAMESPACE______________________________*/
}// namespace files
