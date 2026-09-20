#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"
#include "EVEAudioProcessor.h"

class EVEView : public jam::PluginEditor
{
public:
    EVEView (jam::AudioModel& newModel,
             jam::PluginEditorLayout& newLayout,
             juce::AudioProcessor& processorToConnectTo,
             EVEAudioProcessor& newAudioProcessor);
    ~EVEView() override = default;

private:
    void initialiseTheme() override;
    void initialiseRegistry() override;
    void initialisePanels() override;
    void initialiseView() override;
    void attachPanelCallbacks() override;
    void initialiseListeners() override;
    void fileChanged (const juce::File& file, jam::File::Watcher::Event event) override;

    void resized() override;
    void lookAndFeelChanged() override;

    void setEditorKeys (const jam::ConfigDocument& document);

    //==============================================================================
    EVEAudioProcessor& audioProcessor;
    const juce::File configFile { jam::File::getOrCreateDirectory (juce::File::getSpecialLocation (juce::File::userHomeDirectory), files::configDirectory).getChildFile (files::defaultConfig) };
    jam::AnsiDocument document;
    std::unique_ptr<jam::TextEditor> editor;

    //==============================================================================
#if JUCE_DEBUG
    jam::debug::Log::Scope logScope { jam::File::getDebugLog() };
#endif
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EVEView)
};
