#pragma once

#if JucePlugin_Build_Standalone
#include <JuceHeader.h>
#include "generated/Generated.h"

class EVEStandaloneApp : public jam::AudioStandaloneApp
{
public:
    EVEStandaloneApp() = default;

protected:
    juce::PropertySet* getUserSettings() override;

private:
    juce::PropertiesFile settings { jam::File::getOrCreateDirectory (juce::File::getSpecialLocation (juce::File::userHomeDirectory), files::markdownSettingsDirectory).getChildFile (files::standaloneSettingsFileName), juce::PropertiesFile::Options {} };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EVEStandaloneApp)
};

#endif
