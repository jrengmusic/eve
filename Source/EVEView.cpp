#include "EVEView.h"
#include "EVEProcessor.h"

EVEView::EVEView (jam::AudioModel& newModel,
                  jam::PluginEditorLayout& newLayout,
                  juce::AudioProcessor& processorToConnectTo)
    : jam::PluginEditor (processorToConnectTo, newModel, newLayout)
    , messageOverlay (newModel.getOrCreateComponentState (jam::Format::toTag (Id::message)), [this] { return theme->getMonoFont(); })
{
    registerTerminalEvents();
    initialise (layout.getOrCreateSettings());
    addChildComponent (messageOverlay);
    setConstrainer (this);
    setScaling();

    registry->callbacks.get (jam::Format::toType (Id::message).toString(), static_cast<juce::Component&> (*this));
}

void EVEView::resized()
{
    jam::PluginEditor::resized();
    messageOverlay.setBounds (getLocalBounds());
}

void EVEView::lookAndFeelChanged()
{
    jam::PluginEditor::lookAndFeelChanged();
    model.state.getChildWithName (jam::Format::toType (Id::terminal)).setProperty (Id::background, juce::var { static_cast<juce::int64> (findColour (juce::ResizableWindow::backgroundColourId).getARGB()) }, nullptr);
}

void EVEView::focusGained (FocusChangeType)
{
    if (auto traverser { createKeyboardFocusTraverser() })
    {
        if (auto* defaultComponent { traverser->getDefaultComponent (this) })
            defaultComponent->grabKeyboardFocus();
    }
}

void EVEView::resizeStart() { setResizing (true); }

void EVEView::resizeEnd() { setResizing (false); }

void EVEView::setResizing (bool isResizing)
{
    jam::Component::applyFunctionRecursively (view.get(), [this, isResizing] (juce::Component* child)
    {
        if (jam::Component::hasProperty (child->getProperties(), Id::type, Id::textEditor))
            model.getOrCreateComponentProperty (child->getComponentID(), Id::isResizing).setValue (isResizing);
    });
}

void EVEView::valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property)
{
    if (tree.hasType (jam::Format::toType (Id::terminal)) and terminalEvents.contains (property))
        terminalEvents.get (property, std::as_const (tree));

    jam::PluginEditor::valueTreePropertyChanged (tree, property);
}

void EVEView::registerTerminalEvents()
{
    terminalEvents.add<const juce::ValueTree&> (Id::commandCount, [this] (const juce::ValueTree&)
    {
        getTerminalProcessor().applyCommands ([this] (const juce::Identifier& name) { invokeCommand (name); });
    });

    terminalEvents.add<const juce::ValueTree&> (Id::preview, [this] (const juce::ValueTree& terminal)
    {
        setPreview (terminal.getProperty (Id::preview).toString());
    });

    terminalEvents.add<const juce::ValueTree&> (Id::clipboard, [this] (const juce::ValueTree& terminal)
    {
        getTerminalProcessor().copyToClipboard (static_cast<juce::int64> (terminal.getProperty (Id::clipboard)));
    });

    for (const auto& id : jam::TerminalProcessor::getReports())
        terminalEvents.add<const juce::ValueTree&> (id, [this, id] (const juce::ValueTree& terminal) { applyTerminalReport (terminal, id); });

#if JAM_USING_PROFILER
    terminalEvents.add<const juce::ValueTree&> (Id::promptCount, [this] (const juce::ValueTree& terminal)
    {
        if (static_cast<int> (terminal.getProperty (Id::promptCount)) == 1)
        {
            repaint();

            if (auto* peer { getPeer() })
                peer->performAnyPendingRepaintsNow();

            static_cast<EVEProcessor&> (*getAudioProcessor()).set (Id::promptPainted, [] {});
        }
    });
#endif
}

void EVEView::applyTerminalReport (const juce::ValueTree& terminal, const juce::Identifier& id)
{
    const auto text { terminal.getProperty (id).toString() };

    static_cast<EVEProcessor&> (*getAudioProcessor()).set (Id::terminalReport, [] (const char*, const char*) {}, static_cast<const char*> (id.getCharPointer().getAddress()), text.toRawUTF8());
}

void EVEView::initialiseTheme (const juce::ValueTree& settings)
{
    styleManager.create (layout.fonts, settings);

    theme = std::make_unique<jam::StyleTheme> (*styleManager, model.getAppearance());
    setLookAndFeel (theme.get());
}

void EVEView::initialiseRegistry()
{
    jam::Registry::Registration registration;

    registerButtons (registration);
    registerViewComponents (registration);

    registry.create (registration);
    registerCallbacks();
}

void EVEView::registerButtons (jam::Registry::Registration& registration)
{
    registration.buttons = [] (jam::Registry& r)
    {
        r.registerComponent<jam::ButtonSVG> (Id::button);
        r.registerComponent<jam::ButtonSVG> (Id::up,
                                             juce::StringArray { BinaryData::getString (files::upNormal),
                                                                 BinaryData::getString (files::upOver),
                                                                 BinaryData::getString (files::upDown),
                                                                 juce::String(),
                                                                 juce::String(),
                                                                 juce::String(),
                                                                 juce::String(),
                                                                 juce::String() });
        r.registerComponent<jam::ButtonSVG> (Id::down,
                                             juce::StringArray { BinaryData::getString (files::downNormal),
                                                                 BinaryData::getString (files::downOver),
                                                                 BinaryData::getString (files::downDown),
                                                                 juce::String(),
                                                                 juce::String(),
                                                                 juce::String(),
                                                                 juce::String(),
                                                                 juce::String() });
    };

    registration.config = [] (jam::Registry& r)
    {
        r.registerConfig<jam::Selector> (Id::selector,
                                         [] (jam::Selector* s)
                                         {
                                             auto* reg { jam::Registry::getInstance() };
                                             jassert (reg != nullptr);

                                             s->setNavigationButtons (jam::Registry::toButton (reg->make.get (Id::up)),
                                                                      jam::Registry::toButton (reg->make.get (Id::down)));
                                         });
    };
}

void EVEView::registerViewComponents (jam::Registry::Registration& registration)
{
    registration.viewComponents = [] (jam::Registry& r)
    {
        r.registerComponent<juce::Label> (Id::label);
        r.registerComponent<jam::Selector> (Id::selector);
        r.registerComponent<jam::TextEditor> (Id::textEditor);

        r.registerConfig<jam::Selector> (Id::selector,
                                         jam::Format::toTag (Id::appearance),
                                         [] (jam::Selector* selector)
                                         {
                                             selector->addItemList (map::Appearance::getInstance()->get());
                                         });

        r.registerConfig<jam::Selector> (Id::selector,
                                         jam::Format::toTag (Id::UIScale),
                                         [] (jam::Selector* selector)
                                         {
                                             selector->addItemList (map::UIScaleMap::getInstance()->get());
                                         });
    };

    registration.events = [] (jam::Registry& r)
    {
        r.bindEvent.add<juce::Component*&, jam::ViewManager::Events*, const juce::String&, const juce::String&> (
            Id::textEditor,
            [] (juce::Component* component, jam::ViewManager::Events* chainEvents, const juce::String& eventName, const juce::String&)
            {
                auto* textEditor { static_cast<jam::TextEditor*> (component) };

                textEditor->bindToEvents (chainEvents->at (juce::Identifier { eventName }));
            });
    };

    registration.styles = [] (jam::Registry& r)
    {
        r.registerStyle<jam::StyleTerminal> (Id::textEditor);
    };

    registration.makeContent = [] (jam::Registry& r)
    {
        r.makeContent.add<jam::AudioModel&, juce::LookAndFeel&> (
            Id::settings,
            [] (jam::AudioModel& modelToUse, juce::LookAndFeel& lookAndFeel)
            {
                return jam::ViewSettings::create (modelToUse, lookAndFeel, jam::HtmlDocument::getOrCreate (juce::Identifier { files::settingsLayout }));
            });
    };
}

void EVEView::registerCallbacks()
{
    registry->callbacks.add<juce::Component&> (jam::Format::toType (Id::UIScale),
                                                [] (juce::Component& c)
                                                {
                                                    static_cast<EVEView&> (c).resized();
                                                });

    registry->callbacks.add<juce::Component&> (map::CommandId::getInstance()->get (map::CommandId::clear),
                                                [] (juce::Component& c)
                                                {
                                                    static_cast<EVEView&> (c).clear();
                                                });

    registry->callbacks.add<juce::Component&> (map::CommandId::getInstance()->get (map::CommandId::preview),
                                                [] (juce::Component& c)
                                                {
                                                    auto& editor { static_cast<EVEView&> (c) };
                                                    editor.setPreviewVisible (not editor.isPreviewVisible());
                                                });

    registry->callbacks.add<juce::Component&> (jam::Format::toType (Id::preview),
                                                [] (juce::Component& c)
                                                {
                                                    static_cast<EVEView&> (c).resized();
                                                });

    registry->callbacks.add<juce::Component&> (jam::Format::toType (Id::message),
                                                [] (juce::Component& c)
                                                {
                                                    auto& editor { static_cast<EVEView&> (c) };
                                                    const auto errors { editor.model.getOrCreateComponentProperty (jam::Format::toTag (Id::message), Id::value).toString() };

                                                    if (errors.isNotEmpty())
                                                        editor.messageOverlay.showMessage (errors);
                                                });

    registry->callbacks.add<juce::Component&> (jam::Format::toType (Id::historyCapacity),
                                                [] (juce::Component& c)
                                                {
                                                    auto& editor { static_cast<EVEView&> (c) };

                                                    jam::MessageBox::historyCleared (&editor);
                                                    editor.clear();
                                                });
}

void EVEView::initialiseView()
{
    auto& terminalProcessor { getTerminalProcessor() };

    view = jam::ViewEditor::create<jam::MarkdownDocument> (model, *theme, terminalProcessor.userInterfaceGetters, terminalProcessor.chainEvents, files::viewLayout);
    addAndMakeVisible (view.get());
}

juce::ValueTree EVEView::getViewDefaultKeys() const
{
    auto keys { jam::TextEditor::getDefaultKeys() };
    static_cast<EVEProcessor&> (*getAudioProcessor()).set (Id::keys, [] (juce::ValueTree&) {}, keys);
    return keys;
}

void EVEView::getAllCommands (juce::Array<juce::CommandID>& commands)
{
    jam::PluginEditor::getAllCommands (commands);

    jam::CommandManager::addCommands (commands, *commandId);
}

void EVEView::getCommandInfo (juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    if (jam::Map::contains (map::CommandId::getInstance()->get(), commandID))
    {
        jam::CommandManager::setInfo (result, commandId->get (commandID).toString());
    }
    else
    {
        jam::PluginEditor::getCommandInfo (commandID, result);
    }
}

void EVEView::paintOverChildren (juce::Graphics& g) { transition.paint (g, getLocalBounds()); }

EVETerminalProcessor& EVEView::getTerminalProcessor() const
{
    return static_cast<EVEProcessor&> (*getAudioProcessor()).getTerminalProcessor();
}

void EVEView::clear()
{
    getTerminalProcessor().clear();
}

void EVEView::setPreview (const juce::String& path)
{
    setPreviewVisible (path.isNotEmpty());
}

void EVEView::setPreviewVisible (bool isVisible)
{
    model.getOrCreateComponentProperty (jam::Format::toTag (Id::preview), Id::value).setValue (isVisible);
}

bool EVEView::isPreviewVisible() const
{
    return static_cast<bool> (model.getOrCreateComponentProperty (jam::Format::toTag (Id::preview), Id::value).getValue());
}

void EVEView::invokeCommand (const juce::Identifier& name)
{
    if (const auto* command { commandManager->getCommandForName (name) })
    {
        if (auto traverser { createKeyboardFocusTraverser() })
        {
            if (auto* defaultComponent { dynamic_cast<juce::ApplicationCommandTarget*> (traverser->getDefaultComponent (this)) })
                defaultComponent->invoke (juce::ApplicationCommandTarget::InvocationInfo { command->commandID }, false);
        }
    }
}
