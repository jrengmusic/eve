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
    void fileChanged (const juce::File& file, jam::File::Watcher::Event event) override;

    void initialiseTheme() override;
    void initialiseRegistry() override;
    void initialisePanels() override;
    void initialiseView() override;

    void registerButtons (jam::Registry::Registration& registration);
    void registerViewComponents (jam::Registry::Registration& registration);

    void attachPanelCallbacks() override;
    void attachAboutDialog (jam::ButtonDialog& aboutDialog);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EVEView)
};
