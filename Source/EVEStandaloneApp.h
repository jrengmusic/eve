#pragma once

#if JucePlugin_Build_Standalone
#include <JuceHeader.h>
#include "generated/Generated.h"

class EVEStandaloneApp : public juce::JUCEApplication
{
public:
    EVEStandaloneApp() = default;

    const juce::String getApplicationName() override;
    const juce::String getApplicationVersion() override;
    bool moreThanOneInstanceAllowed() override;
    void initialise (const juce::String& commandLineParameters) override;
    void shutdown() override;
    void systemRequestedQuit() override;

private:
    class MainWindow : public jam::Window
    {
    public:
        MainWindow (std::unique_ptr<juce::Component> editor, const juce::String& name);
        ~MainWindow() override;

        void focusGained (FocusChangeType) override;
        void resizeStart() override;
        void resizeEnd() override;

    private:
        juce::AudioProcessorEditor* getEditor() const noexcept;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

#if JAM_USING_PROFILER
    jam::SharedInstance<jam::Profiler> profiler { std::in_place, jam::Profiler::defaultCapacity };
#endif

    juce::PropertiesFile settings { jam::File::getOrCreateDirectory (juce::File::getSpecialLocation (juce::File::userHomeDirectory), files::markdownSettingsDirectory).getChildFile (files::standaloneSettingsFileName), juce::PropertiesFile::Options {} };

    std::unique_ptr<juce::AudioProcessor> processor;
    std::unique_ptr<MainWindow> mainWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EVEStandaloneApp)
};

#endif
