#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"
#include "EVETerminalProcessor.h"
#include "EVEProcessor.h"

/**
 * @class EVEView
 * @brief Plugin editor of EVE: the terminal view and the markdown components.
 *
 * The view is built from the markdown view layout.
 * The view shows the `jam::TextEditor` of the terminal, and it binds the editor to the terminal processor.
 * The view runs the commands that the shell writes through the command FIFO of the terminal processor.
 * A markdown component shows a parsed document. It is added when the `markdownFile` property of the terminal tree
 * names a file that can be read, and it is removed when that property is empty.
 *
 * @par Thread contract
 * **MESSAGE THREAD** — all methods.
 */
class EVEView
    : public jam::PluginEditor
    , public juce::ComponentBoundsConstrainer
    , public juce::FileDragAndDropTarget
    , public juce::ApplicationCommandTarget
    , private juce::Timer
{
public:
    /**
     * @brief Initialises the editor with the settings of the layout, makes the editor the constrainer of its own size, and sets the scaling.
     *
     * @param newModel                 Model that holds the state of the editor.
     * @param newLayout                Layout documents of the editor.
     * @param processorToConnectTo     Processor that owns the editor. It must be an `EVEProcessor`.
     */
    EVEView (jam::AudioModel& newModel,
             jam::PluginEditorLayout& newLayout,
             juce::AudioProcessor& processorToConnectTo);

    /** @brief Paints the transition over the children. @param g Graphics context to paint into. */
    void paintOverChildren (juce::Graphics& g) override;

    /**
     * @brief Lays out the view.
     *
     * A frame change from outside the window runs no `resizeStart()`. The call closes the winsize gate, and it restarts the coalesce timer when the timer runs or the gate is open.
     */
    void resized() override;

    /** @brief Runs the base class, then writes the background colour of the window to the `background` row of the terminal tree. */
    void lookAndFeelChanged() override;

    /** @brief Applies the appearance of the model to the view and to the ephemeral styles, then runs the base class. */
    void setAppearance() override;

    /**
     * @brief Reloads the settings through the base class, then registers the commands again.
     * @param file   File that changed.
     * @param event  Type of change.
     */
    void fileChanged (const juce::File& file, jam::File::Watcher::Event event) override;

    /**
     * @brief Returns the first command target among the parent components of this view.
     * @return The parent command target, or nullptr when no parent is a command target.
     */
    juce::ApplicationCommandTarget* getNextCommandTarget() override;

    /**
     * @brief Runs the registry callback whose key is the name of the command.
     * @param info  Invocation that carries the command id.
     * @return True when a registry callback exists for the name of the command.
     */
    bool perform (const InvocationInfo& info) override;

private:
    /**
     * @brief Gives the keyboard focus to the default component of the editor.
     * @param cause  Not used.
     */
    void focusGained (FocusChangeType cause) override;

    /**
     * @brief Passes a key to the command manager, together with the markdown state.
     *
     * @param key  Key that the user pressed.
     * @return     `true` when the command manager invoked a command for the key, or when the markdown state has a `mode` property.
     */
    bool keyPressed (const juce::KeyPress& key) override;

    /**
     * @brief Accepts a file drag unless a path holds a control byte, because the shell quote does not escape a control byte.
     * @param files  The absolute paths of the drag.
     * @return       `true` when every path has no control byte.
     */
    bool isInterestedInFileDrag (const juce::StringArray& files) override;

    /**
     * @brief Returns each text editor to terminal mode, and pastes the quoted paths as one text.
     *
     * The pasted text holds no carriage return.
     *
     * @param files  The dropped paths.
     * @param x      Not used.
     * @param y      Not used.
     */
    void filesDropped (const juce::StringArray& files, int x, int y) override;

    /**
     * @brief Runs as the constrainer of this editor when a user resize starts. It marks the text editors as resizing, so that they keep the winsize unchanged until the resize ends.
     *
     * It stops the coalesce timer, because the live resize now owns the gate.
     */
    void resizeStart() override;

    /**
     * @brief Ends the resize. It clears the resizing mark of the text editors.
     *
     * It writes the width and the height of the view to the `width` and `height` rows of the terminal tree.
     * It stops the coalesce timer. The timer also runs this call as the final stop of a frame change from outside the window.
     */
    void resizeEnd() override;

    /**
     * @brief Writes `Id::isResizing` to the component property of each text editor in the view.
     * @param newIsResizing  `true` holds the winsize unchanged.
     */
    void setResizing (bool newIsResizing);

    /**
     * @brief Answers whether any text editor holds the winsize gate closed. It reads the `Id::isResizing` property of the value tree of each text editor, and creates nothing.
     *
     * An absent `Id::isResizing` property reads as `false`.
     *
     * @return `true` when the `Id::isResizing` property of any text editor is true.
     */
    bool isResizing() const;

    /**
     * @brief Ends a coalesced frame change by running `resizeEnd()`.
     */
    void timerCallback() override;

    /**
     * @brief Reacts to a change of a property in the model tree.
     *
     * A change of a property of the `terminal` tree runs the handler of `terminalEvents` for that property, when a handler exists.
     * The handlers cover `Id::numCommands`, `Id::clipboard`, `Id::shellExited`, `Id::zoom`, `Id::width`, `Id::height`,
     * `Id::focus`, `Id::markdownFile`, and, when `JAM_USING_PROFILER` is set, `Id::numPrompts`.
     *
     * The base class gets every change.
     *
     * @param tree      Tree that changed.
     * @param property  Property that changed.
     */
    void valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property) override;

    /**
     * @brief Builds the style manager and the theme, and sets the theme as the LookAndFeel.
     * @param settings  Settings tree of the editor.
     */
    void initialiseTheme (const juce::ValueTree& settings) override;

    /** @brief Builds the registry with the buttons, the view components, and the callbacks. */
    void initialiseRegistry() override;

    /** @brief Builds the view from the markdown view layout and the getters and events of the terminal processor, then adds the view. */
    void initialiseView() override;

    /**
     * @brief Makes the editor resizable, then sets its first size from the width and height rows of the terminal tree.
     *
     * The window is made resizable without the bottom-right corner resizer. The function writes no row.
     */
    void setScaling() override;

    /**
     * @brief Registers the commands of the view with the command manager, and sets the key presses from the keys sheet.
     *
     * A view default key is added only when the config holds no key of the same type. Then every config key is appended to the keys sheet.
     * The call registers the commands of each child of the view, then of the view itself, sets the key presses,
     * and makes the view want keyboard focus.
     * It runs after initialise() in the constructor, and again after each settings file change in fileChanged().
     */
    void addCommands();

    /**
     * @brief Writes the zoom row of the terminal tree, limited with juce::jlimit to the zoom range of this view.
     * @param newZoom  Zoom value to write.
     */
    void setZoom (float newZoom);

    /**
     * @brief Returns the default keys of the text editor for the view commands.
     *
     * The keys come from jam::TextEditor::getDefaultKeys(). The call then passes them to the
     * processor through `set (Id::keys, ...)` with an empty callback.
     *
     * @return The default key sheet of the text editor.
     */
    juce::ValueTree getViewDefaultKeys() const;

    /**
     * @brief Adds the id of every command of the `map::CommandId` map.
     * @param commands  Array that receives the ids.
     */
    void getAllCommands (juce::Array<juce::CommandID>& commands) override;

    /**
     * @brief Describes a command by the name of its `map::CommandId` entry.
     * @param commandID  Command to describe.
     * @param result     Info to fill.
     */
    void getCommandInfo (juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;

    /**
     * @brief Registers the buttons, and the configuration of the selector that uses the buttons.
     * @param registration  Registration that receives the buttons.
     */
    void registerButtons (jam::Registry::Registration& registration);

    /**
     * @brief Registers the view components, their configurations, their events, their styles and the content of the settings.
     *
     * The event of the text editor binds the terminal input of the editor to the callback of the chain events (`TextEditor::bindToEvents()`).
     *
     * @param registration  Registration that receives the view components.
     */
    void registerViewComponents (jam::Registry::Registration& registration);

    /**
     * @brief Adds the registry callbacks of the UI scale, the commands, the message, and the history capacity.
     *
     * The UI scale callback runs resized(). The clear command runs clear(). The open file command runs openFile().
     * The markdown command writes the path of the URL to the `markdownFile` row of the terminal tree.
     * The dismiss command writes an empty `markdownFile` row.
     * The zoomIn command adds the `zoomStep` of the display sheet to the zoom row, and the zoomOut command subtracts it, both through setZoom().
     * The zoomReset command sets the zoom row to 1.0 through setZoom().
     * The message callback shows the message of the message component state when its text is not empty.
     * The history capacity callback shows the history-cleared message box and runs clearHistory().
     */
    void registerCallbacks();

    /** @brief Clears the terminal. On the alternate screen, the terminal processor sends a form feed; otherwise it clears the history. */
    void clear();

    /** @brief Clears the history of the terminal processor. */
    void clearHistory();

    /**
     * @brief Returns the open command that the `openWith` sheet gives for a file.
     *
     * The key is `directory` for a folder. For a file, the key is the extension, or the file name when the file has no extension.
     * The `any` entry is the fallback when the sheet has no entry for the key.
     *
     * @param file  File to open.
     * @return      Command text of the entry. The string is empty when the sheet has no entry.
     */
    juce::String getOpenCommand (const juce::File& file) const;

    /**
     * @brief Opens the url of the terminal. A local file goes to `openFile (path, terminal)`. Any other url opens in the default browser.
     */
    void openFile();

    /**
     * @brief Opens a decoded file path of the terminal.
     *
     * A missing file shows a message. An existing file runs its command when the command is a registered command.
     * Otherwise the file starts as a process when the open command is empty, when the terminal records or uses the alternate screen, or when the path holds a control byte.
     * Otherwise the text editor returns to terminal mode, so that the program gets the keys, and the shell receives the open command with the path.
     *
     * @param path      Decoded file path to open.
     * @param terminal  The `terminal` tree. It gives the recording and alternate screen state.
     */
    void openFile (std::string_view path, const juce::ValueTree& terminal);

    /**
     * @brief Returns the local file that the url of the terminal names.
     *
     * The file is empty when the url is not a local file.
     *
     * @return File of the decoded url path. An empty file when the url is not a local file.
     */
    juce::File getUrlFile() const;

    /**
     * @brief Adds a markdown component that shows the file, and parses the file into its document.
     *
     * A file whose memory mapping gives no data clears the `markdownFile` row and shows a message. Otherwise the
     * component is made visible and placed in front, and the `mode` property of the markdown state is set to panel.
     *
     * @param file             Markdown file to show.
     * @param shouldGrabFocus  Passed to the front call of the markdown component.
     */
    void addMarkdown (const juce::File& file, bool shouldGrabFocus);

    /**
     * @brief Runs the callback on each text editor of the view.
     *
     * The call finds each child by its `type` property.
     *
     * @tparam Callback  Callable that takes a `jam::TextEditor&`.
     * @param callback   Runs once for each text editor.
     */
    template <typename Callback>
    void applyToTextEditors (Callback&& callback) const
    {
        applyToChildren<jam::TextEditor> (Id::textEditor, std::forward<Callback> (callback));
    }

    /**
     * @brief Runs the callback on the view and on each of its descendants whose `type` property holds the text of @p type.
     *
     * The walk is depth-first, and it visits the view before its children.
     * The cast to `ChildType` is not checked. The caller names the type that the `type` property denotes.
     *
     * @tparam ChildType  Component type of the matching children.
     * @tparam Callback   Callable that takes a `ChildType&`.
     * @param type        Value of the `type` property to match.
     * @param callback    Runs once for each matching component.
     */
    template <typename ChildType, typename Callback>
    void applyToChildren (const juce::Identifier& type, Callback&& callback) const
    {
        jam::Component::applyFunctionRecursively (view.get(), [&type, &callback] (juce::Component* child)
        {
            if (jam::Component::hasProperty (child->getProperties(), Id::type, type))
                callback (*static_cast<ChildType*> (child));
        });
    }

    /**
     * @brief Returns the processor that owns the editor, cast to `EVEProcessor`.
     *
     * The cast is static. The constructor requires that the owner processor is an `EVEProcessor`.
     *
     * @return The `EVEProcessor` of the editor.
     */
    EVEProcessor& getEVEProcessor() const;

    /**
     * @brief Returns the `terminal` child of the model state.
     *
     * The result is a reference to that tree, not a copy. A write through the result changes the model.
     * The result is invalid when the model state has no `terminal` child.
     *
     * @return The `terminal` tree of the model.
     */
    juce::ValueTree getTerminalState() const;

    /**
     * @brief Returns the markdown child of the model state, created when absent.
     * @return The markdown state tree of the model.
     */
    juce::ValueTree getOrCreateMarkdownState() const;

    /**
     * @brief Returns the terminal processor of the owner EVEProcessor.
     * @return The terminal processor.
     */
    EVETerminalProcessor& getTerminalProcessor() const;

    /**
     * @brief Invokes the command of the command manager with a name on each text editor of the view.
     *
     * Does nothing when the command manager has no command with that name.
     *
     * @param name  Name of the command.
     */
    void invokeCommand (const juce::Identifier& name);

    /**
     * @brief Adds the handlers of the `terminal` tree properties to `terminalEvents`.
     *
     * The `numCommands` handler runs the commands of the command FIFO through invokeCommand().
     * The `clipboard` handler copies the image record that the property names to the system clipboard.
     * The `shellExited` handler shows the `Id::failShellExited` message when the property is true, then invokes the quit command of the command manager.
     * The `zoom` handler sets the zoom of the view and lays it out again.
     * The `width` handler sets the width of the view and keeps its height.
     * The `height` handler sets the height of the view and keeps its width.
     * The `focus` handler grabs the keyboard focus when the property is true, and gives it away when the property is false.
     * The `markdownFile` handler clears the ephemeral components and their styles. It then adds the markdown component
     * of the file when the property is not empty, and otherwise removes the `mode` property of the markdown state.
     * When `JAM_USING_PROFILER` is set, the `numPrompts` handler repaints the view when the count is 1, and it sets the `Id::promptPainted` callback of the processor.
     */
    void registerTerminalEvents();

    /** Shared instance of the `map::CommandId` map. */
    jam::SharedInstance<map::CommandId> commandId { std::in_place };

    /** Command manager of the view. It registers the commands of the view and matches the key presses against the key sheet from addCommands(). */
    jam::SharedInstance<jam::CommandManager> commandManager { std::in_place };

    /** Overlay that shows the settings message. It shows the message when the message callback runs with a non-empty MESSAGE component property. */
    jam::MessageOverlay messageOverlay;

    jam::Function::Map<juce::Identifier, void> terminalEvents;    ///< Handlers of the `terminal` tree properties, keyed by property identifier. `valueTreePropertyChanged()` runs them.
    /** @brief Styles of the ephemeral components. Each style is owned here and cleared with the components. */
    jam::Owner<jam::StyleCustom> ephemeralStyles;

    /** @brief Ephemeral components of the view, such as the markdown components. Each is owned here. */
    jam::Owner<juce::Component> ephemerals;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EVEView)
};
