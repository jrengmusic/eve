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
    const juce::String configDirectory  { juce::String::fromUTF8 (".config/end") };
    const juce::String defaultConfig    { juce::String::fromUTF8 ("eve.md") };
    const juce::String parametersLayout { juce::String::fromUTF8 ("parameters.md") };

/**______________________________END OF NAMESPACE______________________________*/
}// namespace files
