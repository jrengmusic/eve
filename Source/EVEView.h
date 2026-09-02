#pragma once
#include <JuceHeader.h>

class EVEView : public jam::PluginEditor
{
public:
    EVEView (jam::AudioModel& newModel, jam::PluginEditorLayout& newLayout, juce::AudioProcessor& processorToConnectTo);
    ~EVEView() override;

private:
    void initialiseTheme() override;
    void initialiseView() override;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EVEView)
};
