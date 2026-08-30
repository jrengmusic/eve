#pragma once
#include <JuceHeader.h>

class ENDView : public jam::PluginEditor
{
public:
    ENDView (jam::AudioModel& newModel, jam::PluginEditorLayout& newLayout, juce::AudioProcessor& processorToConnectTo);
    ~ENDView() override = default;

private:
    void initialiseTheme() override;
    void initialiseView() override;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ENDView)
};
