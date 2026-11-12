#include "EVEView.h"
#include "ShellIntegration.h"

EVEView::EVEView (jam::AudioModel& newModel,
                  jam::PluginEditorLayout& newLayout,
                  juce::AudioProcessor& processorToConnectTo)
    : jam::PluginEditor (processorToConnectTo, newModel, newLayout)
    , messageOverlay (newModel.getOrCreateComponentState (jam::Format::toTag (Id::message)), [this] { return theme->getMonoFont(); })
{
    registerTerminalEvents();
    initialise (layout.getOrCreateSettings());
    addCommands();
    addChildComponent (messageOverlay);
    setConstrainer (this);
    setScaling();

    registry->callbacks.get (jam::Format::toType (Id::message).toString(), static_cast<juce::Component&> (*this));
    getTerminalState().setProperty (Id::markdownFile, juce::String {}, nullptr);
}

void EVEView::resized()
{
    if (isTimerRunning() or not isResizing())
    {
        setResizing (true);
        startTimer (coalesceWindowTimeMs);
    }

    const auto zoom { static_cast<float> (getTerminalState().getProperty (Id::zoom)) };

    view->setBounds (jam::ViewEditor::getViewBounds (model, getLocalBounds(), zoom));
    view->setTransform (juce::AffineTransform::scale (zoom));
    messageOverlay.setBounds (getLocalBounds());

    for (const auto& ephemeral : ephemerals)
        ephemeral->setBounds (getLocalBounds());
}

void EVEView::lookAndFeelChanged()
{
    jam::PluginEditor::lookAndFeelChanged();
    getTerminalState().setProperty (Id::background, juce::var { static_cast<juce::int64> (findColour (juce::ResizableWindow::backgroundColourId).getARGB()) }, nullptr);
}

void EVEView::setAppearance()
{
    jam::ViewManager::setAppearance (view.get(), ephemeralStyles, model.getAppearance());
    jam::PluginEditor::setAppearance();
}

bool EVEView::isInterestedInFileDrag (const juce::StringArray& files)
{
    return std::none_of (files.begin(), files.end(), [] (const juce::String& file) { return ShellIntegration::hasControlByte (file.toRawUTF8()); });
}

void EVEView::filesDropped (const juce::StringArray& files, int, int)
{
    const auto text { getTerminalProcessor().getQuotedPaths (files) };

    applyToTextEditors ([&text] (jam::TextEditor& editor)
    {
        editor.setMode (map::EditorMode::terminal);
        editor.applyPaste (text);
    });
}

void EVEView::focusGained (FocusChangeType)
{
    if (auto traverser { createKeyboardFocusTraverser() })
    {
        if (auto* defaultComponent { traverser->getDefaultComponent (this) })
            defaultComponent->grabKeyboardFocus();
    }
}

void EVEView::parentHierarchyChanged()
{
    if (isShowing() and static_cast<bool> (getTerminalState().getProperty (Id::focus)))
        grabKeyboardFocus();
}

bool EVEView::keyPressed (const juce::KeyPress& key)
{
    auto markdownState { getOrCreateMarkdownState() };

    return commandManager->invokeKey (key, markdownState, false) or markdownState.hasProperty (Id::mode);
}

void EVEView::resizeStart()
{
    stopTimer();
    setResizing (true);
}

void EVEView::resizeEnd()
{
    stopTimer();
    setResizing (false);

    if (not jam::PluginHostType().isEND())
    {
        auto terminal { getTerminalState() };

        terminal.setProperty (Id::width, getWidth(), nullptr);
        terminal.setProperty (Id::height, getHeight(), nullptr);
    }
}

void EVEView::setResizing (bool newIsResizing)
{
    applyToTextEditors ([this, newIsResizing] (jam::TextEditor& editor)
    {
        model.getOrCreateComponentProperty (editor.getComponentID(), Id::isResizing).setValue (newIsResizing);
    });
}

bool EVEView::isResizing() const
{
    bool isAnyResizing { false };

    applyToTextEditors ([&isAnyResizing] (jam::TextEditor& editor)
    {
        isAnyResizing = isAnyResizing or static_cast<bool> (editor.getValueTree().getProperty (Id::isResizing));
    });

    return isAnyResizing;
}

void EVEView::timerCallback() { resizeEnd(); }

void EVEView::valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property)
{
    if (tree.hasType (jam::Format::toType (Id::terminal)) and terminalEvents.contains (property))
        terminalEvents.get (property, tree);

    jam::PluginEditor::valueTreePropertyChanged (tree, property);
}

void EVEView::registerTerminalEvents()
{
    terminalEvents.add (Id::numCommands, [this] (const juce::Identifier&, const juce::ValueTree&)
    {
        getTerminalProcessor().applyCommands ([this] (const juce::Identifier& name) { invokeCommand (name); });
    });

    terminalEvents.add (Id::numGridEvents, [this] (const juce::Identifier&, const juce::ValueTree&)
    {
        getTerminalProcessor().applyGridEvents();
    });

    terminalEvents.add (Id::clipboard, [this] (const juce::Identifier&, const juce::ValueTree& terminal)
    {
        getTerminalProcessor().copyToClipboard (static_cast<juce::int64> (terminal.getProperty (Id::clipboard)));
    });

    terminalEvents.add (Id::shellExited, [this] (const juce::Identifier&, const juce::ValueTree& terminal)
    {
        if (static_cast<bool> (terminal.getProperty (Id::shellExited)))
        {
            messageOverlay.showMessage (Id::failShellExited);
            commandManager->invokeDirectly (juce::StandardApplicationCommandIDs::quit, true);
        }
    });

    terminalEvents.add (Id::shellNotFound, [this] (const juce::Identifier&, const juce::ValueTree& terminal)
    {
        const auto program { terminal.getProperty (Id::shellNotFound).toString() };

        if (program.isNotEmpty())
            messageOverlay.showMessage (Id::failShellNotFound + program);
    });

    terminalEvents.add (Id::zoom, [this] (const juce::Identifier&, const juce::ValueTree& terminal)
    {
        setZoom (static_cast<float> (terminal.getProperty (Id::zoom)));
        resized();
    });

    terminalEvents.add (Id::width, [this] (const juce::Identifier&, const juce::ValueTree& terminal)
    {
        setSize (static_cast<int> (terminal.getProperty (Id::width)), getHeight());
    });

    terminalEvents.add (Id::height, [this] (const juce::Identifier&, const juce::ValueTree& terminal)
    {
        setSize (getWidth(), static_cast<int> (terminal.getProperty (Id::height)));
    });

    terminalEvents.add (Id::focus, [this] (const juce::Identifier&, const juce::ValueTree& terminal)
    {
        if (static_cast<bool> (terminal.getProperty (Id::focus)))
            grabKeyboardFocus();
        else
            giveAwayKeyboardFocus();
    });

    terminalEvents.add (Id::markdownFile, [this] (const juce::Identifier&, const juce::ValueTree& terminal)
    {
        const auto path { terminal.getProperty (Id::markdownFile).toString() };
        const auto hadFocus { hasKeyboardFocus (true) };

        ephemerals.clear();
        ephemeralStyles.clear();

        if (path.isNotEmpty())
        {
            addMarkdown (juce::File { path }, hadFocus);
        }
        else
        {
            getOrCreateMarkdownState().removeProperty (Id::mode, nullptr);

            if (hadFocus)
                grabKeyboardFocus();
        }
    });

#if JAM_USING_PROFILER
    terminalEvents.add (Id::numPrompts, [this] (const juce::Identifier&, const juce::ValueTree& terminal)
    {
        if (static_cast<int> (terminal.getProperty (Id::numPrompts)) == 1)
        {
            repaint();

            if (auto* peer { getPeer() })
                peer->performAnyPendingRepaintsNow();

            getEVEProcessor().set (Id::promptPainted, [] {});
        }
    });
#endif
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

static void setNavigationButtons (jam::Selector* s)
{
    auto* reg { jam::Registry::getInstance() };
    jassert (reg != nullptr);

    s->setNavigationButtons (jam::Registry::toButton (reg->make.get (Id::up)),
                             jam::Registry::toButton (reg->make.get (Id::down)));
}

static void addAppearanceItemList (jam::Selector* selector)
{
    selector->addItemList (map::Appearance::getInstance()->get());
}

static void addUIScaleItemList (jam::Selector* selector)
{
    selector->addItemList (map::UIScaleMap::getInstance()->get());
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
        r.registerConfig<jam::Selector, &setNavigationButtons> (Id::selector);
    };
}

void EVEView::registerViewComponents (jam::Registry::Registration& registration)
{
    registration.viewComponents = [] (jam::Registry& r)
    {
        r.registerComponent<juce::Label> (Id::label);
        r.registerComponent<jam::Selector> (Id::selector);
        r.registerComponent<jam::TextEditor> (Id::textEditor);
        r.registerComponent<jam::PreviewComponent> (Id::previewComponent);
        r.registerComponent<jam::TextEditor> (Id::markdown);

        r.registerConfig<jam::Selector, &addAppearanceItemList> (Id::selector, jam::Format::toTag (Id::appearance));
        r.registerConfig<jam::Selector, &addUIScaleItemList> (Id::selector, jam::Format::toTag (Id::UIScale));
    };

    registration.events = [] (jam::Registry& r)
    {
        r.bindEvent.add (
            Id::textEditor,
            [] (const juce::String&, juce::Component* component, jam::ViewManager::Events* chainEvents, const juce::String& eventName, const juce::String&)
            {
                auto* textEditor { static_cast<jam::TextEditor*> (component) };

                textEditor->bindToEvents (chainEvents->at (juce::Identifier { eventName }));
            });
    };

    registration.styles = [] (jam::Registry& r)
    {
        r.registerStyle<jam::StyleTerminal> (Id::textEditor);
        r.registerStyle<jam::StyleMarkdown> (Id::markdown);
    };

    registration.makeContent = [] (jam::Registry& r)
    {
        r.makeContent.add (
            Id::settings,
            [] (const juce::String&, jam::AudioModel& modelToUse, juce::LookAndFeel& lookAndFeel)
            {
                return jam::ViewSettings::create (modelToUse, lookAndFeel, jam::HtmlDocument::getOrCreate (juce::Identifier { files::settingsLayout }));
            });
    };
}

void EVEView::registerCallbacks()
{
    registry->callbacks.add (jam::Format::toType (Id::UIScale),
                             [] (const juce::String&, juce::Component& c)
                             {
                                 static_cast<EVEView&> (c).resized();
                             });

    registry->callbacks.add (map::CommandId::getInstance()->get (map::CommandId::openFile),
                             [] (const juce::String&, juce::Component& c)
                             {
                                 static_cast<EVEView&> (c).openFile();
                             });

    registry->callbacks.add (map::CommandId::getInstance()->get (map::CommandId::markdown),
                             [] (const juce::String&, juce::Component& c)
                             {
                                 auto& editor { static_cast<EVEView&> (c) };
                                 const auto url { editor.getUrl() };

                                 if (url.isLocalFile())
                                     editor.getTerminalState().setProperty (Id::markdownFile, url.getLocalFile().getFullPathName(), nullptr);
                             });

    registry->callbacks.add (map::CommandId::getInstance()->get (map::CommandId::dismiss),
                             [] (const juce::String&, juce::Component& c)
                             {
                                 static_cast<EVEView&> (c).getTerminalState().setProperty (Id::markdownFile, juce::String {}, nullptr);
                             });

    registry->callbacks.add (map::CommandId::getInstance()->get (map::CommandId::zoomIn),
                             [] (const juce::String&, juce::Component& c)
                             {
                                 auto& editor { static_cast<EVEView&> (c) };

                                 editor.setZoom (static_cast<float> (editor.getTerminalState().getProperty (Id::zoom)) + static_cast<float> (editor.styleManager->getConfig (jam::StyleManager::getDisplaySheet()).getChildWithName (jam::Format::toType (Id::zoomStep)).getProperty (Id::value)));
                             });

    registry->callbacks.add (map::CommandId::getInstance()->get (map::CommandId::zoomOut),
                             [] (const juce::String&, juce::Component& c)
                             {
                                 auto& editor { static_cast<EVEView&> (c) };

                                 editor.setZoom (static_cast<float> (editor.getTerminalState().getProperty (Id::zoom)) - static_cast<float> (editor.styleManager->getConfig (jam::StyleManager::getDisplaySheet()).getChildWithName (jam::Format::toType (Id::zoomStep)).getProperty (Id::value)));
                             });

    registry->callbacks.add (map::CommandId::getInstance()->get (map::CommandId::zoomReset),
                             [] (const juce::String&, juce::Component& c)
                             {
                                 static_cast<EVEView&> (c).setZoom (1.0f);
                             });

    registry->callbacks.add (jam::Format::toType (Id::message),
                             [] (const juce::String&, juce::Component& c)
                             {
                                 auto& editor { static_cast<EVEView&> (c) };
                                 const auto errors { editor.model.getOrCreateComponentProperty (jam::Format::toTag (Id::message), Id::value).toString() };

                                 if (errors.isNotEmpty())
                                     editor.messageOverlay.showMessage (errors);
                             });

    registry->callbacks.add (jam::Format::toType (Id::historyCapacity),
                             [] (const juce::String&, juce::Component& c)
                             {
                                 auto& editor { static_cast<EVEView&> (c) };

                                 jam::MessageBox::historyCleared (&editor);
                                 editor.clearHistory();
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
    getEVEProcessor().set (Id::keys, [] (juce::ValueTree&) {}, keys);
    return keys;
}

void EVEView::getAllCommands (juce::Array<juce::CommandID>& commands)
{
    jam::CommandManager::addCommands (commands, *commandId);
}

void EVEView::getCommandInfo (juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    jam::CommandManager::setInfo (result, commandId->get (commandID).toString());
}

void EVEView::setScaling()
{
    const auto terminal { getTerminalState() };

    setResizable (true, false);
    setSize (static_cast<int> (terminal.getProperty (Id::width)), static_cast<int> (terminal.getProperty (Id::height)));
}

void EVEView::setZoom (float newZoom)
{
    static constexpr float minimumZoom { 0.25f };
    static constexpr float maximumZoom { 4.0f };

    getTerminalState().setProperty (Id::zoom, juce::jlimit (minimumZoom, maximumZoom, newZoom), nullptr);
}

void EVEView::addCommands()
{
    const auto keysSheet { jam::Format::toType (Id::keys) };
    const auto config { styleManager->getConfig (keysSheet) };
    juce::ValueTree keys { keysSheet };

    for (const auto& defaultKey : getViewDefaultKeys())
        if (not config.getChildWithName (jam::Format::toType (defaultKey.getType())).isValid())
            keys.appendChild (defaultKey.createCopy(), nullptr);

    for (const auto& configKey : config)
        keys.appendChild (configKey.createCopy(), nullptr);

    jam::Component::applyFunctionRecursively (view.get(), [this] (juce::Component* child)
    {
        commandManager->registerAllCommandsForTarget (dynamic_cast<juce::ApplicationCommandTarget*> (child));
    });

    commandManager->registerAllCommandsForTarget (this);
    commandManager->setKeyPresses (keys);
    setWantsKeyboardFocus (true);
}

void EVEView::fileChanged (const juce::File& file, jam::File::Watcher::Event event)
{
    jam::PluginEditor::fileChanged (file, event);
    addCommands();
}

juce::ApplicationCommandTarget* EVEView::getNextCommandTarget()
{
    return findFirstTargetParentComponent();
}

bool EVEView::perform (const InvocationInfo& info)
{
    const auto name { commandManager->getNameOfCommand (info.commandID) };
    const auto isPerformed { registry->callbacks.contains (name) };

    if (isPerformed)
        registry->callbacks.get (name, static_cast<juce::Component&> (*this));

    return isPerformed;
}

void EVEView::paintOverChildren (juce::Graphics& g) { transition.paint (g, getLocalBounds()); }

EVEProcessor& EVEView::getEVEProcessor() const
{
    return static_cast<EVEProcessor&> (*getAudioProcessor());
}

EVETerminalProcessor& EVEView::getTerminalProcessor() const
{
    return getEVEProcessor().getTerminalProcessor();
}

juce::ValueTree EVEView::getTerminalState() const
{
    return model.state.getChildWithName (jam::Format::toType (Id::terminal));
}

juce::ValueTree EVEView::getOrCreateMarkdownState() const
{
    return model.getOrCreateComponentState (jam::Format::toTag (Id::markdown));
}

void EVEView::clearHistory()
{
    getTerminalProcessor().clearHistory();
}

juce::String EVEView::getOpenCommand (const juce::File& file) const
{
    const auto table { styleManager->getConfig (jam::Format::toType (Id::openWith)) };
    const auto extension { file.getFileExtension() };
    const auto key { file.isDirectory() ? jam::Format::toType (Id::directory)
                                        : jam::Format::toType (juce::Identifier { extension.isEmpty() ? file.getFileName() : extension }) };

    return table.getProperty (key, table.getProperty (jam::Format::toType (Id::any))).toString();
}

void EVEView::openFile()
{
    const auto url { getUrl() };

    if (url.isLocalFile())
        openFile (url.getLocalFile(), getTerminalState());
    else
        url.launchInDefaultBrowser();
}

void EVEView::openFile (const juce::File& file, const juce::ValueTree& terminal)
{
    const auto path { file.getFullPathName() };
    const auto value { getOpenCommand (file) };
    const auto* command { value.isEmpty() ? nullptr : commandManager->getCommandForName (juce::Identifier { value }) };
    const auto isBusy { static_cast<bool> (terminal.getProperty (Id::isRecording)) or static_cast<bool> (terminal.getProperty (Id::alternateScreen)) };

    if (file.exists())
    {
        if (command != nullptr) commandManager->invokeDirectly (command->commandID, false);
        else if (value.isEmpty() or isBusy or ShellIntegration::hasControlByte (path.toRawUTF8())) file.startAsProcess();
        else
        {
            applyToTextEditors ([] (jam::TextEditor& editor) { editor.setMode (map::EditorMode::terminal); });
            getTerminalProcessor().applyOpenCommand (value.toRawUTF8(), path.toRawUTF8());
        }
    }
    else
    {
        messageOverlay.showMessage (Id::failNoOpenFile + file.getFileName().quoted (Chars::singleQuote));
    }
}

juce::URL EVEView::getUrl() const
{
    return juce::URL { getTerminalState().getProperty (Id::url).toString() };
}

void EVEView::addMarkdown (const juce::File& file, bool shouldGrabFocus)
{
    static constexpr int firstFocusOrder { 1 };

    const juce::MemoryMappedFile source { file, juce::MemoryMappedFile::readOnly };

    if (source.getData() != nullptr)
    {
        const auto markdownType { Id::markdown.toString() };
        auto* markdown { static_cast<jam::TextEditor*> (jam::ViewManager::addComponent (ephemerals,
                                                                                               ephemeralStyles,
                                                                                               jam::Registry::getInstance(),
                                                                                               markdownType,
                                                                                               juce::String {},
                                                                                               markdownType,
                                                                                               juce::String {},
                                                                                               juce::String {},
                                                                                               model.getAppearance(),
                                                                                               markdownType)) };

        addAndMakeVisible (markdown);
        markdown->setBounds (getLocalBounds());
        markdown->setDocument (jam::MarkdownDocument::parse (std::string_view { static_cast<const char*> (source.getData()), source.getSize() }, file.getFullPathName()));
        markdown->setWantsKeyboardFocus (true);
        markdown->setExplicitFocusOrder (firstFocusOrder);
        markdown->toFront (shouldGrabFocus);
        messageOverlay.toFront (false);
        getOrCreateMarkdownState().setProperty (Id::mode, static_cast<int> (map::EditorMode::panel), nullptr);
    }
    else
    {
        getTerminalState().setProperty (Id::markdownFile, juce::String {}, nullptr);
        messageOverlay.showMessage (Id::failNoOpenFile + file.getFileName().quoted (Chars::singleQuote));
    }
}

void EVEView::invokeCommand (const juce::Identifier& name)
{
    if (const auto* command { commandManager->getCommandForName (name) })
    {
        applyToTextEditors ([command] (jam::TextEditor& editor)
        {
            editor.invoke (juce::ApplicationCommandTarget::InvocationInfo { command->commandID }, false);
        });
    }
}
