#include "EVEView.h"

static const juce::String defaultFixture {
    "/Users/jreng/Documents/Poems/dev/eve/tests/ansi_fixpoint/fixtures/ls.ansi"
};

static juce::String getFixtureText()
{
    auto parameters { juce::JUCEApplicationBase::getCommandLineParameterArray() };
    parameters.add (defaultFixture);

    for (const auto& parameter : parameters)
    {
        const auto fixtureFile { juce::File::getCurrentWorkingDirectory().getChildFile (parameter) };

        if (fixtureFile.existsAsFile())
            return fixtureFile.loadFileAsString();
    }

    return {};
}

/**
 * @brief Builds the Document::Index::Codec pair backing the index's release
 *        tier -- encodes a line's cells to ANSI wire bytes, decodes wire
 *        bytes back into cells via a fresh single-line AnsiDocument parse.
 * @return The encode/decode pair.
 */
static jam::Document::Index::Codec getCodec()
{
    return { [] (const jam::Document::Element& element) -> juce::MemoryBlock
             {
                 const auto* cells { element.get<jam::Document::Cells> (Id::cells) };
                 const auto rowText { jam::terminal::getRowText (cells->data(), cells->size()) };

                 return juce::MemoryBlock (rowText.data(), rowText.size());
             },
             [] (jam::Document::Element& element, const juce::MemoryBlock& wireBytes)
             {
                 const auto lineDocument { jam::AnsiDocument::parse (
                     juce::String::fromUTF8 (static_cast<const char*> (wireBytes.getData()),
                                             static_cast<int> (wireBytes.getSize()))) };
                 auto* cells { element.get<jam::Document::Cells> (Id::cells) };

                 if (auto* decodedLine { lineDocument.getRoot()->firstChild })
                      *cells = std::move (*decodedLine->get<jam::Document::Cells> (Id::cells));
                 else
                     *cells = jam::Document::Cells {};
             } };
}

struct ChoiceText : public juce::Value::ValueSource, private juce::Value::Listener
{
    ChoiceText (juce::Value sourceToUse, juce::StringArray choicesToUse)
        : source { std::move (sourceToUse) }
        , choices { std::move (choicesToUse) }
    {
        source.addListener (this);
    }

    juce::var getValue() const override
    {
        return choices[static_cast<int> (source.getValue())];
    }

    void setValue (const juce::var& newValue) override
    {
        source = choices.indexOf (newValue.toString());
    }

    void valueChanged (juce::Value&) override
    {
        sendChangeMessage (true);
    }

    juce::Value source;
    juce::StringArray choices;
};

static jam::ConfigDocument getConfigDocument (const juce::File& configFile)
{
    auto document { jam::ConfigDocument::parse (configFile.loadFileAsString(), configFile.getFullPathName()) };

    if (jam::ConfigValidator::isValid (document).failed())
        document = jam::ConfigDocument::parse (BinaryData::getString (files::defaultConfig), files::defaultConfig);

    return document;
}

EVEView::EVEView (jam::AudioModel& newModel,
                  jam::PluginEditorLayout& newLayout,
                  juce::AudioProcessor& processorToConnectTo,
                  EVEAudioProcessor& newAudioProcessor)
    : jam::PluginEditor (processorToConnectTo,
                         newModel,
                         newLayout,
                         jam::ViewManager::getUISize<jam::MarkdownDocument> (
                             juce::Identifier { files::defaultConfig }))
    , audioProcessor { newAudioProcessor }
    , document { jam::AnsiDocument::parse (getFixtureText()) }
{
    if (layout.isReady (model, configFile))
    {
        initialise();

        setResizable (true, true);

        const auto [width, height] { view->getUISize (model) };
        setSize (width, height);
    }
}

void EVEView::initialiseTheme()
{
    const auto document { getConfigDocument (configFile) };

    styleManager.create (layout.fonts,
                         document.getValueTree (Id::toType (Id::config)),
                         document.getValueTree (Id::toType (Id::config), Id::dark));

    theme = std::make_unique<jam::StyleTheme> (*styleManager, model.getAppearance());
    juce::LookAndFeel::setDefaultLookAndFeel (theme.get());
}

void EVEView::initialiseRegistry()
{
    jam::Registry::Registration registration;

    registration.viewComponents = [] (jam::Registry& r)
    {
        r.registerComponent<juce::Label> (Id::label);
    };

    registration.viewBindings = [] (jam::Registry& r)
    {
        r.registerViewComponent (
            jam::Format::toScreamingSnakeCase (Id::mode.toString()),
            [] (juce::Component* c, const jam::Document::Element& element, jam::Registry&, jam::AudioModel& modelToUse)
            {
                auto* label { static_cast<juce::Label*> (c) };
                const juce::Identifier parameterID { jam::Registry::getParameter (element) };
                const juce::Identifier property { jam::Registry::getContent (element) };

                if (property == Id::value)
                    label->getTextValue().referTo (juce::Value { new ChoiceText (
                        modelToUse.getParameterAsValue (parameterID.toString()),
                        static_cast<juce::AudioParameterChoice*> (modelToUse.getParameter (parameterID.toString()))->choices) });
                else
                    label->getTextValue().referTo (modelToUse.getValue (parameterID, property));
            });
    };

    registry.create (registration);
}

void EVEView::initialisePanels()
{
    panel = jam::ViewPanel::create (model, jam::HtmlDocument::getOrCreate (juce::Identifier { files::panelLayout }));
    addAndMakeVisible (panel.get());
}

void EVEView::initialiseView()
{
    view = jam::ViewEditor::create<jam::MarkdownDocument> (model, audioProcessor.userInterfaceGetters, audioProcessor.chainEvents, files::defaultConfig);
    addAndMakeVisible (view.get());

    editor = std::make_unique<jam::TextEditor> (document,
                                                getCodec(),
                                                model.state.getChildWithProperty (Id::id, jam::Format::toScreamingSnakeCase (Id::mode.toString())));
    addAndMakeVisible (*editor);

    setEditorKeys (getConfigDocument (configFile));
}

void EVEView::setEditorKeys (const jam::ConfigDocument& document)
{
    for (const auto& [mode, name] : map::EditorMode::getInstance()->get())
        editor->setKeys (mode, document.getValueTree (Id::toType (Id::config), juce::Identifier { name })
                                   .getChildWithName (Id::toType (Id::keys)));
}

void EVEView::attachPanelCallbacks() {}

void EVEView::initialiseListeners()
{
    settingsWatcher.addFolder (configFile.getParentDirectory());
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
            setEditorKeys (document);
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
    panel->setBounds (getLocalBounds());
    view->setBounds (view->getViewBounds (model));
    editor->setBounds (getLocalBounds().withTrimmedBottom (model.getUIPanelHeight()).reduced (theme->getWindowPadding()));
}
