#include "EVEView.h"
#include "ShellIntegration.h"

EVEView::EVEView (jam::AudioModel& newModel,
                  jam::PluginEditorLayout& newLayout,
                  juce::AudioProcessor& processorToConnectTo)
    : jam::PluginEditor (processorToConnectTo, newModel, newLayout)
    , messageOverlay (newModel.getOrCreateComponentState (jam::Format::toTag (Id::message)), [this] { return theme->getMonoFont(); })
{
    auto markdownState { model.getOrCreateComponentState (jam::Format::toTag (Id::markdown)) };

    markdownState.setProperty (Id::mode, static_cast<int> (map::EditorMode::panel), nullptr);
    markdownState.setProperty (Id::value, false, nullptr);

    registerTerminalEvents();
    initialise (layout.getOrCreateSettings());
    addCommands();
    addChildComponent (messageOverlay);
    setConstrainer (this);
    setScaling();

    registry->callbacks.get (jam::Format::toType (Id::message).toString(), static_cast<juce::Component&> (*this));

    if (isListMode())
        setPanelsVisible (true);
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

    if (not panels.isEmpty())
    {
        const auto& left { getPanel (leftPanel) };
        const auto& right { getPanel (rightPanel) };

        if (left.isPanelShowing())
            view->setBounds (view->getBounds().withTrimmedLeft (view->getLocalArea (this, left.getBounds()).getWidth()));

        if (right.isPanelShowing())
            view->setBounds (view->getBounds().withTrimmedRight (view->getLocalArea (this, right.getBounds()).getWidth()));
    }
}

void EVEView::lookAndFeelChanged()
{
    jam::PluginEditor::lookAndFeelChanged();
    getTerminalState().setProperty (Id::background, juce::var { static_cast<juce::int64> (findColour (juce::ResizableWindow::backgroundColourId).getARGB()) }, nullptr);
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

bool EVEView::keyPressed (const juce::KeyPress& key)
{
    auto markdownState { model.getOrCreateComponentState (jam::Format::toTag (Id::markdown)) };

    return commandManager->invokeKey (key, markdownState);
}

void EVEView::resizeStart()
{
    stopTimer();
    setResizing (true);
}

void EVEView::resizeEnd()
{
    stopTimer();
    setResizing (isListMode());

    auto terminal { getTerminalState() };

    terminal.setProperty (Id::width, getWidth(), nullptr);
    terminal.setProperty (Id::height, getHeight(), nullptr);
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

bool EVEView::isListMode() const
{
    return static_cast<int> (getTerminalState().getProperty (Id::mode)) == map::EditorMode::list;
}

void EVEView::timerCallback() { resizeEnd(); }

void EVEView::valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property)
{
    if (tree.hasType (jam::Format::toType (Id::terminal)) and terminalEvents.contains (property))
        terminalEvents.get (property, std::as_const (tree));

    jam::PluginEditor::valueTreePropertyChanged (tree, property);
}

void EVEView::registerTerminalEvents()
{
    terminalEvents.add<const juce::ValueTree&> (Id::numCommands, [this] (const juce::ValueTree&)
    {
        getTerminalProcessor().applyCommands ([this] (const juce::Identifier& name) { invokeCommand (name); });
    });

    terminalEvents.add<const juce::ValueTree&> (Id::clipboard, [this] (const juce::ValueTree& terminal)
    {
        getTerminalProcessor().copyToClipboard (static_cast<juce::int64> (terminal.getProperty (Id::clipboard)));
    });

    terminalEvents.add<const juce::ValueTree&> (Id::url, [this] (const juce::ValueTree&) { setPanelFile(); });

    terminalEvents.add<const juce::ValueTree&> (Id::numYanks, [this] (const juce::ValueTree&)
    {
        applyToTextEditors ([] (jam::TextEditor& editor) { editor.setMode (map::EditorMode::terminal); });
    });

    terminalEvents.add<const juce::ValueTree&> (Id::shellExited, [this] (const juce::ValueTree& terminal)
    {
        if (static_cast<bool> (terminal.getProperty (Id::shellExited)))
        {
            messageOverlay.showMessage (Id::failShellExited);
            commandManager->invokeDirectly (juce::StandardApplicationCommandIDs::quit, true);
        }
    });

    terminalEvents.add<const juce::ValueTree&> (Id::zoom, [this] (const juce::ValueTree& terminal)
    {
        setZoom (static_cast<float> (terminal.getProperty (Id::zoom)));
        resized();
    });

    terminalEvents.add<const juce::ValueTree&> (Id::width, [this] (const juce::ValueTree& terminal)
    {
        setSize (static_cast<int> (terminal.getProperty (Id::width)), getHeight());
    });

    terminalEvents.add<const juce::ValueTree&> (Id::height, [this] (const juce::ValueTree& terminal)
    {
        setSize (getWidth(), static_cast<int> (terminal.getProperty (Id::height)));
    });

    terminalEvents.add<const juce::ValueTree&> (Id::focus, [this] (const juce::ValueTree& terminal)
    {
        if (static_cast<bool> (terminal.getProperty (Id::focus)))
            grabKeyboardFocus();
        else
            giveAwayKeyboardFocus();
    });

    terminalEvents.add<const juce::ValueTree&> (Id::mode, [this] (const juce::ValueTree& terminal)
    {
        const auto isList { static_cast<int> (terminal.getProperty (Id::mode)) == map::EditorMode::list };

        if (isList != (not panels.isEmpty() and getPanel (leftPanel).isPanelShowing()))
            setPanelsVisible (isList);
    });

#if JAM_USING_PROFILER
    terminalEvents.add<const juce::ValueTree&> (Id::numPrompts, [this] (const juce::ValueTree& terminal)
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
        r.registerComponent<jam::PreviewComponent> (Id::previewComponent);
        r.registerComponent<jam::MarkdownComponent> (Id::markdown);

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

        r.registerConfig<jam::MarkdownComponent> (Id::markdown,
                                                  [] (jam::MarkdownComponent* markdown)
                                                  {
                                                      markdown->setVisible (false);
                                                      markdown->setWantsKeyboardFocus (true);
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
        r.registerStyle<jam::StyleMarkdown> (Id::markdown);
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

    registry->callbacks.add<juce::Component&> (map::CommandId::getInstance()->get (map::CommandId::openFile),
                                                [] (juce::Component& c)
                                                {
                                                    static_cast<EVEView&> (c).openFile();
                                                });

    registry->callbacks.add<juce::Component&> (map::CommandId::getInstance()->get (map::CommandId::markdown),
                                                [] (juce::Component& c)
                                                {
                                                    static_cast<EVEView&> (c).openMarkdown();
                                                });

    registry->callbacks.add<juce::Component&> (map::CommandId::getInstance()->get (map::CommandId::dismiss),
                                                [] (juce::Component& c)
                                                {
                                                    static_cast<EVEView&> (c).setMarkdownVisible (false);
                                                });

    registry->callbacks.add<juce::Component&> (map::CommandId::getInstance()->get (map::CommandId::zoomIn),
                                                [] (juce::Component& c)
                                                {
                                                    auto& editor { static_cast<EVEView&> (c) };

                                                    editor.setZoom (static_cast<float> (editor.getTerminalState().getProperty (Id::zoom)) + static_cast<float> (editor.styleManager->getConfig (jam::StyleManager::getDisplaySheet()).getChildWithName (jam::Format::toType (Id::zoomStep)).getProperty (Id::value)));
                                                });

    registry->callbacks.add<juce::Component&> (map::CommandId::getInstance()->get (map::CommandId::zoomOut),
                                                [] (juce::Component& c)
                                                {
                                                    auto& editor { static_cast<EVEView&> (c) };

                                                    editor.setZoom (static_cast<float> (editor.getTerminalState().getProperty (Id::zoom)) - static_cast<float> (editor.styleManager->getConfig (jam::StyleManager::getDisplaySheet()).getChildWithName (jam::Format::toType (Id::zoomStep)).getProperty (Id::value)));
                                                });

    registry->callbacks.add<juce::Component&> (map::CommandId::getInstance()->get (map::CommandId::zoomReset),
                                                [] (juce::Component& c)
                                                {
                                                    static_cast<EVEView&> (c).setZoom (1.0f);
                                                });

    registry->callbacks.add<juce::Component&> (jam::Format::toType (Id::markdown),
                                                [] (juce::Component& c)
                                                {
                                                    auto& editor { static_cast<EVEView&> (c) };
                                                    const auto isVisible { static_cast<bool> (editor.model.getOrCreateComponentProperty (jam::Format::toTag (Id::markdown), Id::value).getValue()) };

                                                    editor.applyToMarkdown ([isVisible] (jam::MarkdownComponent& markdown)
                                                    {
                                                        markdown.setVisible (isVisible);

                                                        if (isVisible)
                                                            markdown.grabKeyboardFocus();
                                                    });

                                                    if (not isVisible)
                                                        editor.grabKeyboardFocus();
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

void EVEView::clear()
{
    getTerminalProcessor().clear();
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
    const auto terminal { getTerminalState() };
    const juce::URL url { terminal.getProperty (Id::url).toString() };

    if (url.isLocalFile())
    {
        const auto path { getUrlFile().getFullPathName() };

        openFile (path.toRawUTF8(), terminal);
    }
    else
    {
        url.launchInDefaultBrowser();
    }
}

void EVEView::openFile (std::string_view path, const juce::ValueTree& terminal)
{
    const juce::File file { juce::String::fromUTF8 (path.data(), static_cast<int> (path.size())) };
    const auto value { getOpenCommand (file) };
    const auto* command { value.isEmpty() ? nullptr : commandManager->getCommandForName (juce::Identifier { value }) };
    const auto isBusy { static_cast<bool> (terminal.getProperty (Id::isRecording)) or static_cast<bool> (terminal.getProperty (Id::alternateScreen)) };

    if (file.exists())
    {
        if (command != nullptr) commandManager->invokeDirectly (command->commandID, false);
        else if (value.isEmpty() or isBusy or ShellIntegration::hasControlByte (path)) file.startAsProcess();
        else
        {
            applyToTextEditors ([] (jam::TextEditor& editor) { editor.setMode (map::EditorMode::terminal); });
            getTerminalProcessor().applyOpenCommand (value.toRawUTF8(), path);
        }
    }
    else
    {
        messageOverlay.showMessage (Id::failNoOpenFile + file.getFileName().quoted (Chars::singleQuote));
    }
}

void EVEView::openMarkdown()
{
    const auto file { getUrlFile() };
    const juce::MemoryMappedFile source { file, juce::MemoryMappedFile::readOnly };

    if (source.getData() != nullptr)
    {
        applyToMarkdown ([&source, &file] (jam::MarkdownComponent& markdown)
        {
            markdown.setDocument (jam::MarkdownDocument::parse (std::string_view { static_cast<const char*> (source.getData()), source.getSize() }, file.getFullPathName()));
        });

        setMarkdownVisible (true);
    }
}

void EVEView::setMarkdownVisible (bool isVisible)
{
    model.getOrCreateComponentProperty (jam::Format::toTag (Id::markdown), Id::value).setValue (isVisible);
}

void EVEView::selectionChanged()
{
    const auto& tree { static_cast<const juce::FileTreeComponent&> (*getPanel (leftPanel).getContent()) };

    if (tree.getNumSelectedFiles() > 0)
        getTerminalState().setProperty (Id::url, juce::URL { tree.getSelectedFile() }.toString (false), nullptr);
}

void EVEView::fileClicked (const juce::File&, const juce::MouseEvent&) {}

void EVEView::fileDoubleClicked (const juce::File&) { openFile(); }

void EVEView::browserRootChanged (const juce::File&) {}

juce::SidePanel& EVEView::getPanel (int index) const
{
    return static_cast<juce::SidePanel&> (*panels.at (index));
}

juce::File EVEView::getTreeRoot() const
{
    const auto display { styleManager->getConfig (jam::StyleManager::getDisplaySheet()) };
    const auto depth { static_cast<int> (display.getChildWithName (jam::Format::toType (Id::treeDepth)).getProperty (Id::value)) };
    juce::File root { getTerminalState().getProperty (Id::cwd).toString() };

    for (int level { 0 }; level < depth; ++level)
        root = root.getParentDirectory();

    return root;
}

juce::File EVEView::getUrlFile() const
{
    const auto url { getTerminalState().getProperty (Id::url).toString() };
    juce::File file;

    if (juce::URL { url }.isLocalFile())
    {
        jam::TerminalProcessor::applyToDecodedText (jam::TerminalProcessor::getUriPath (url.toRawUTF8()), [&file] (std::string_view path)
        {
            file = juce::File { juce::String::fromUTF8 (path.data(), static_cast<int> (path.size())) };
        });
    }

    return file;
}

void EVEView::addPanels()
{
    const auto display { styleManager->getConfig (jam::StyleManager::getDisplaySheet()) };
    const auto getPanelWidth { [this, &display] (const juce::Identifier& row)
    {
        return juce::roundToInt (static_cast<float> (getWidth()) * static_cast<float> (display.getChildWithName (jam::Format::toType (row)).getProperty (Id::value)));
    } };
    auto tree { std::make_unique<juce::FileTreeComponent> (directoryList) };

    tree->addListener (this);
    panels.add (std::make_unique<jam::FilePreview> (model.getOrCreateComponentState (jam::Format::toTag (Id::preview))));

    applyToMarkdown ([this] (jam::MarkdownComponent& markdown)
    {
        panels.at (filePreview)->setLookAndFeel (&markdown.getLookAndFeel());
    });

    panels.add (std::make_unique<juce::SidePanel> (juce::String {}, getPanelWidth (Id::treeWidth), true, tree.release(), true));
    panels.add (std::make_unique<juce::SidePanel> (juce::String {}, getPanelWidth (Id::previewWidth), false, panels.at (filePreview).get(), false));

    for (const auto index : { leftPanel, rightPanel })
    {
        auto& panel { getPanel (index) };

        panel.setTitleBarHeight (0);
        juce::Desktop::getInstance().removeGlobalMouseListener (&panel);
        panel.onPanelShowHide = [this] (bool) { resized(); };
        addChildComponent (panel);
    }

    directoryThread.startThread();
}

void EVEView::setPanelsVisible (bool isVisible)
{
    if (isVisible and panels.isEmpty())
        addPanels();

    if (not panels.isEmpty())
    {
        if (isVisible)
        {
            setResizing (true);
            directoryList.setDirectory (getTreeRoot(), true, true);
        }

        getPanel (leftPanel).showOrHide (isVisible);
        getPanel (rightPanel).showOrHide (isVisible);
        resized();

        if (not isVisible)
            setResizing (isTimerRunning());

        setPanelFile();
    }
}

void EVEView::setPanelFile()
{
    if (not panels.isEmpty() and getPanel (leftPanel).isPanelShowing())
    {
        const auto file { getUrlFile() };

        if (file.exists())
        {
            static_cast<juce::FileTreeComponent&> (*getPanel (leftPanel).getContent()).setSelectedFile (file);
            static_cast<jam::FilePreview&> (*panels.at (filePreview)).setFile (file);
        }
    }
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
