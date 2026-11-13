#if JucePlugin_Build_Standalone
#include "EVEStandaloneApp.h"
#include "EVETerminalProcessor.h"

EVEStandaloneApp::MainWindow::MainWindow (std::unique_ptr<juce::Component> editor, const juce::String& name)
    : jam::Window (std::move (editor), name, false, true)
{
    juce::LookAndFeel::setDefaultLookAndFeel (&getContentComponent()->getLookAndFeel());

    setVisible (true);
}

EVEStandaloneApp::MainWindow::~MainWindow()
{
    if (auto* editor { getEditor() })
        editor->processor.editorBeingDeleted (editor);

    clearContentComponent();

    juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
}

void EVEStandaloneApp::MainWindow::focusGained (FocusChangeType)
{
    if (auto* editor { getEditor() })
        editor->grabKeyboardFocus();
}

void EVEStandaloneApp::MainWindow::resizeStart()
{
    if (auto* constrainer { getEditor()->getConstrainer() })
        constrainer->resizeStart();
}

void EVEStandaloneApp::MainWindow::resizeEnd()
{
    if (auto* constrainer { getEditor()->getConstrainer() })
        constrainer->resizeEnd();
}

juce::AudioProcessorEditor* EVEStandaloneApp::MainWindow::getEditor() const noexcept
{
    return dynamic_cast<juce::AudioProcessorEditor*> (getContentComponent());
}

const juce::String EVEStandaloneApp::getApplicationName() { return ProjectInfo::projectName; }

const juce::String EVEStandaloneApp::getApplicationVersion() { return ProjectInfo::versionString; }

bool EVEStandaloneApp::moreThanOneInstanceAllowed() { return true; }

void EVEStandaloneApp::initialise (const juce::String&)
{
#if JAM_USING_PROFILER
    const jam::Profiler::Zone zone { Id::standaloneInitialise };
#endif
    {
#if JAM_USING_PROFILER
        const jam::Profiler::Zone engineZone { Id::graphicsEngine };
#endif
        jam::VulkanGraphicsEngine::getOrCreate();
    }

    processor = juce::createPluginFilterOfType (juce::AudioProcessor::wrapperType_Standalone);

    juce::MemoryBlock state;

    if (state.fromBase64Encoding (settings.getValue (Id::pluginState)) and state.getSize() > 0)
        processor->setStateInformation (state.getData(), static_cast<int> (state.getSize()));

    processor->prepareToPlay (processor->getSampleRate(), processor->getBlockSize());
    mainWindow = std::make_unique<MainWindow> (std::unique_ptr<juce::Component> (processor->createEditorAndMakeActive()), getApplicationName());
}

void EVEStandaloneApp::systemRequestedQuit()
{
    juce::MemoryBlock state;
    processor->getStateInformation (state);
    settings.setValue (Id::pluginState, state.toBase64Encoding());
    quit();
}

void EVEStandaloneApp::shutdown()
{
    {
#if JAM_USING_PROFILER
        const jam::Profiler::Zone zone { Id::standaloneShutdown };
#endif
        mainWindow = nullptr;
        processor->releaseResources();
        processor = nullptr;
    }
#if JAM_USING_PROFILER
    profiler->drain();
    profiler->writeSamples (EVETerminalProcessor::getManifestFile(), jam::Format::toType (juce::Identifier { ProjectInfo::projectName }));
#endif
}

juce::JUCEApplicationBase* juce_CreateApplication() { return new EVEStandaloneApp(); }

#endif
