#include "EVEView.h"

static const juce::String defaultFixture {
    "/Users/jreng/Documents/Poems/dev/eve/tests/ansi_fixpoint/fixtures/ls.ansi"
};

EVEView::EVEView (jam::AudioModel& newModel,
                  jam::PluginEditorLayout& newLayout,
                  juce::AudioProcessor& processorToConnectTo,
                  EVEAudioProcessor& newAudioProcessor,
                  jam::TerminalModel& newTerminalModel)
    : jam::PluginEditor (processorToConnectTo,
                         newModel,
                         newLayout,
                         jam::ViewManager::getUISize<jam::MarkdownDocument> (
                             juce::Identifier { files::defaultConfig }))
    , audioProcessor { newAudioProcessor }
    , terminalModel { newTerminalModel }
    , messageOverlay (newTerminalModel,
                      newTerminalModel.state.getChildWithName (Id::toType (Id::overlay)),
                      [this] { return juce::Font { theme->getMonoFont() }; })
{
    const auto ready { layout.isReady (model, configFile) };

    initialise();

    setResizable (true, true);

    const auto [width, height] { view->getUISize (model) };
    setSize (width, height);

    if (not ready)
    {
        const auto validation { jam::ConfigValidator::isValid (
            jam::ConfigDocument::parse (configFile.loadFileAsString(), configFile.getFullPathName())) };

        if (validation.failed())
        {
            auto* messageParam { terminalModel.getParameter<jam::ParameterText> (Id::toType (Id::overlay), Id::message) };
            jassert (messageParam != nullptr);

            if (messageParam != nullptr)
                messageParam->setValue (validation.getErrorMessage());
        }
    }
}

void EVEView::initialiseTheme()
{
    auto document { jam::ConfigDocument::parse (configFile.loadFileAsString(), configFile.getFullPathName()) };

    if (jam::ConfigValidator::isValid (document).failed())
        document = jam::ConfigDocument::parse (BinaryData::getString (files::defaultConfig), files::defaultConfig);

    styleManager.create (layout.fonts,
                         document.getValueTree (Id::toType (Id::config)),
                         document.getValueTree (Id::toType (Id::config), Id::dark));

    theme = std::make_unique<jam::StyleTheme> (*styleManager, model.getAppearance());
    juce::LookAndFeel::setDefaultLookAndFeel (theme.get());
}

void EVEView::initialiseRegistry() {}

void EVEView::initialisePanels() {}

void EVEView::initialiseView()
{
    view = jam::ViewEditor::create<jam::MarkdownDocument> (model, audioProcessor.userInterfaceGetters, audioProcessor.chainEvents, files::defaultConfig);
    addAndMakeVisible (view.get());

    initialiseTerminalView();

    addChildComponent (messageOverlay);
}

void EVEView::initialiseTerminalView()
{
    auto parameters { juce::JUCEApplicationBase::getCommandLineParameterArray() };
    parameters.add (defaultFixture);

    for (const auto& parameter : parameters)
    {
        const auto fixtureFile { juce::File::getCurrentWorkingDirectory().getChildFile (parameter) };

        if (fixtureFile.existsAsFile())
        {
            terminalView = std::make_unique<jam::TextEditor> (fixtureFile.loadFileAsString(), terminalModel, terminalModel.state, jam::UUID::none());
            break;
        }
    }

    jassert (terminalView != nullptr);

    if (terminalView != nullptr)
    {
        addAndMakeVisible (*terminalView);

        jam::Model::Attachment { *terminalView };
    }
}

void EVEView::attachPanelCallbacks() {}

void EVEView::initialiseListeners()
{
    settingsWatcher.addFolder (configFile.getParentDirectory());

    messageOverlay.registerParameters();
}

void EVEView::fileChanged (const juce::File& file, jam::File::Watcher::Event event)
{
    if (file == configFile and event != jam::File::Watcher::fileDeleted)
    {
        const auto document { jam::ConfigDocument::parse (configFile.loadFileAsString(), configFile.getFullPathName()) };
        const auto validation { jam::ConfigValidator::isValid (document) };

        if (validation.wasOk())
        {
            styleManager->registerStyle (document.getValueTree (Id::toType (Id::config)),
                                         document.getValueTree (Id::toType (Id::config), Id::dark));
            theme->setAppearance (model.getAppearance());
            sendLookAndFeelChange();
        }
        else
        {
            auto* messageParam { terminalModel.getParameter<jam::ParameterText> (Id::toType (Id::overlay), Id::message) };
            jassert (messageParam != nullptr);

            if (messageParam != nullptr)
                messageParam->setValue (validation.getErrorMessage());
        }
    }
}

void EVEView::lookAndFeelChanged()
{
    jam::PluginEditor::lookAndFeelChanged();
    resized();
}

void EVEView::resized()
{
    view->setBounds (view->getViewBounds (model));
    terminalView->setBounds (getLocalBounds().reduced (theme->getWindowPadding()));
    messageOverlay.setBounds (getLocalBounds());
}
