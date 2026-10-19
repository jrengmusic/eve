#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"

class EVEView : public jam::PluginEditor
{
public:
    EVEView (jam::AudioModel& newModel,
             jam::PluginEditorLayout& newLayout,
             juce::AudioProcessor& processorToConnectTo);

    void paintOverChildren (juce::Graphics& g) override;

private:
    void initialiseTheme (const juce::ValueTree& settings) override;
    void initialiseRegistry() override;
    void initialiseView() override;

    void registerButtons (jam::Registry::Registration& registration);
    void registerViewComponents (jam::Registry::Registration& registration);

    void attachPanelCallbacks() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EVEView)
};
