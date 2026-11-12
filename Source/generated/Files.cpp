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

    const juce::String viewLayout                 { juce::String::fromUTF8 ("default-eve.md") };
    const juce::String settingsLayout             { juce::String::fromUTF8 ("SettingsLayout.html") };
    const juce::String parametersLayout           { juce::String::fromUTF8 ("parameters.md") };
    const juce::String profileReport              { juce::String::fromUTF8 ("profile.md") };
    const juce::String markdownSettingsDirectory  { juce::String::fromUTF8 (JUCE_STRINGIFY (CAST_CONFIG_DIRECTORY)) };
    const juce::String markdownSettingsFileName   { juce::String::fromUTF8 ("eve/display.md") };
    const juce::String standaloneSettingsFileName { juce::String::fromUTF8 ("EVE.settings") };
    const juce::String upNormal                   { juce::String::fromUTF8 ("up_normal.svg") };
    const juce::String upOver                     { juce::String::fromUTF8 ("up_over.svg") };
    const juce::String upDown                     { juce::String::fromUTF8 ("up_down.svg") };
    const juce::String downNormal                 { juce::String::fromUTF8 ("down_normal.svg") };
    const juce::String downOver                   { juce::String::fromUTF8 ("down_over.svg") };
    const juce::String downDown                   { juce::String::fromUTF8 ("down_down.svg") };
    const juce::String shellIntegrationDirectory  { juce::String::fromUTF8 ("shell") };
    const juce::String conptyDirectory            { juce::String::fromUTF8 ("conpty") };
    const juce::String conptyLibrary              { juce::String::fromUTF8 ("conpty.dll") };
    const juce::String openConsole                { juce::String::fromUTF8 ("OpenConsole.exe") };
    const juce::String zshEnvironment             { juce::String::fromUTF8 ("zsh_zshenv.zsh") };
    const juce::String zshIntegration             { juce::String::fromUTF8 ("zsh_integration.zsh") };
    const juce::String bashIntegration            { juce::String::fromUTF8 ("bash_integration.bash") };
    const juce::String powershellIntegration      { juce::String::fromUTF8 ("powershell_integration.ps1") };
    const juce::String shellCommand               { juce::String::fromUTF8 ("eve") };

/**______________________________END OF NAMESPACE______________________________*/
}// namespace files
