#include "EVEView.h"
#include "EVEProcessor.h"

EVEView::EVEView (jam::AudioModel& newModel,
                  jam::PluginEditorLayout& newLayout,
                  juce::AudioProcessor& processorToConnectTo)
    : jam::PluginEditor (processorToConnectTo, newModel, newLayout)
    , messageOverlay (newModel.getOrCreateComponentState (Id::toTag (Id::message)), [this] { return theme->getMonoFont(); })
{
    initialise (layout.getOrCreateSettings());
    addChildComponent (messageOverlay);
    setConstrainer (this);
    setScaling();

    registry->callbacks.get (Id::toType (Id::message).toString(), static_cast<juce::Component&> (*this));
}

void EVEView::resized()
{
    jam::PluginEditor::resized();
    messageOverlay.setBounds (getLocalBounds());
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
    if (property == Id::commandCount)
        applyCommands();

    if (property == Id::preview)
        setPreview (tree.getProperty (property).toString());

#if JAM_USING_PROFILER
    if (property == Id::promptCount and static_cast<int> (tree.getProperty (property)) == 1)
    {
        repaint();

        auto* peer { getPeer() };
        jassert (peer != nullptr);

        if (peer != nullptr)
            peer->performAnyPendingRepaintsNow();

        const auto* extensions { dynamic_cast<const jam::ENAXClientExtensions*> (getAudioProcessor()) };
        jassert (extensions != nullptr);

        if (extensions != nullptr)
            extensions->set (Id::promptPainted, [] {});
    }
#endif

    jam::PluginEditor::valueTreePropertyChanged (tree, property);
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
                                         Id::toTag (Id::appearance),
                                         [] (jam::Selector* selector)
                                         {
                                             selector->addItemList (map::Appearance::getInstance()->get());
                                         });

        r.registerConfig<jam::Selector> (Id::selector,
                                         Id::toTag (Id::UIScale),
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
        r.registerStyle<jam::StyleCustom> (Id::textEditor);
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
    registry->callbacks.add<juce::Component&> (Id::toType (Id::UIScale),
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

    registry->callbacks.add<juce::Component&> (Id::toType (Id::preview),
                                                [] (juce::Component& c)
                                                {
                                                    static_cast<EVEView&> (c).resized();
                                                });

    registry->callbacks.add<juce::Component&> (Id::toType (Id::message),
                                                [] (juce::Component& c)
                                                {
                                                    auto& editor { static_cast<EVEView&> (c) };
                                                    const auto errors { editor.model.getOrCreateComponentProperty (Id::toTag (Id::message), Id::value).toString() };

                                                    if (errors.isNotEmpty())
                                                        editor.messageOverlay.showMessage (errors);
                                                });

    registry->callbacks.add<juce::Component&> (Id::toType (Id::historyCapacity),
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

void EVEView::getAllCommands (juce::Array<juce::CommandID>& commands)
{
    jam::PluginEditor::getAllCommands (commands);

    for (const auto& [command, name] : map::CommandId::getInstance()->get())
        commands.add (command);
}

void EVEView::getCommandInfo (juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    if (jam::Map::contains (map::CommandId::getInstance()->get(), commandID))
    {
        const auto name { map::CommandId::getInstance()->get (commandID).toString() };
        result.setInfo (name, name, {}, result.flags & ~ juce::ApplicationCommandInfo::isDisabled);
    }
    else
    {
        jam::PluginEditor::getCommandInfo (commandID, result);
    }
}

void EVEView::paintOverChildren (juce::Graphics& g) { transition.paint (g, getLocalBounds()); }

EVETerminalProcessor& EVEView::getTerminalProcessor()
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
    model.getOrCreateComponentProperty (Id::toTag (Id::preview), Id::value).setValue (isVisible);
}

bool EVEView::isPreviewVisible() const
{
    return static_cast<bool> (model.getOrCreateComponentProperty (Id::toTag (Id::preview), Id::value).getValue());
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

void EVEView::applyCommands()
{
    juce::String cut {};

    getTerminalProcessor().getCommands().applyPendingData ([this, &cut] (const char* data, int size)
    {
        const char* const end { data + size };

        while (data < end)
        {
            const char* const terminator { std::find (data, end, '\0') };

            if (terminator == end)
                cut += juce::String { juce::String::CharPointerType (data), juce::String::CharPointerType (end) };
            else
            {
                const auto name { cut + juce::String { juce::String::CharPointerType (data), juce::String::CharPointerType (terminator) } };

                cut = juce::String {};

                if (name.isNotEmpty())
                    invokeCommand (juce::Identifier { name });
            }

            data = juce::jmin (terminator + 1, end);
        }
    });
}
