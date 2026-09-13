#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"

class EVEView : public jam::PluginEditor
{
public:
    EVEView (jam::AudioModel& newModel,
             jam::PluginEditorLayout& newLayout,
             juce::AudioProcessor& processorToConnectTo);
    ~EVEView() override = default;

private:
    void initialiseTheme() override;
    void initialiseRegistry() override;
    void initialisePanels() override;
    void initialiseView() override;
    void attachPanelCallbacks() override;
    void initialiseListeners() override;

    //==============================================================================
#if JUCE_DEBUG
    jam::debug::Log::Scope logScope { jam::File::getDebugLog() };
#endif
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EVEView)
};
