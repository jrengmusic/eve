#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"
#include "EVETerminalProcessor.h"

/**
 * @class EVEView
 * @brief Plugin editor of EVE: the terminal view, built from the markdown view layout.
 *
 * The view shows the `jam::TextEditor` of the terminal, and it binds the editor to the terminal processor.
 * A change of the preview property of the model shows or hides the side preview.
 * The view runs the commands that the shell writes through the command FIFO of the terminal processor.
 *
 * @par Thread contract
 * **MESSAGE THREAD** — all methods.
 */
class EVEView
    : public jam::PluginEditor
    , public juce::ComponentBoundsConstrainer
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

    void resized() override;

    void lookAndFeelChanged() override;

private:
    /**
     * @brief Gives the keyboard focus to the default component of the editor.
     * @param cause  Not used.
     */
    void focusGained (FocusChangeType cause) override;

    /** @brief Runs as the constrainer of this editor when a user resize starts. It marks the text editors as resizing, so that they keep the winsize unchanged until the resize ends. */
    void resizeStart() override;

    /** @brief Clears the resizing mark of the text editors, so that they write the winsize of the final size. */
    void resizeEnd() override;

    /**
     * @brief Writes `Id::isResizing` to the component property of each text editor in the view.
     * @param isResizing  `true` while the user resizes the window.
     */
    void setResizing (bool isResizing);

    /**
     * @brief Reacts to a change of a property in the model tree.
     *
     * A change of a property of the `terminal` tree runs the handler of `terminalEvents` for that property, when a handler exists.
     * The handlers cover `Id::commandCount`, `Id::preview`, `Id::clipboard`, the report properties of `jam::TerminalProcessor::getReports()`,
     * and, in a profile build, `Id::promptCount`. The base class gets every change.
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
     * @brief Returns the default keys of the text editor for the view commands.
     *
     * The keys come from jam::TextEditor::getDefaultKeys().
     * The processor then runs the `Id::keys` rule of its ENAX client extensions on the keys.
     * Under END, that rule removes the `normal` and `prefix` rows.
     * Under any other host, the rule is not registered, so the keys stay unchanged.
     *
     * @return The default key sheet of the text editor.
     */
    juce::ValueTree getViewDefaultKeys() const override;

    /**
     * @brief Adds the id of every command of the editor: the commands of the plugin editor and the commands of `map::CommandId`.
     * @param commands  Array that receives the ids.
     */
    void getAllCommands (juce::Array<juce::CommandID>& commands) override;

    /**
     * @brief Describes a command of `map::CommandId` by its name. Any other command goes to the plugin editor.
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

    /** @brief Adds the registry callbacks of the UI scale and the preview, the clear and the preview commands, and the history capacity. */
    void registerCallbacks();

    /** @brief Clears the screen and the saved lines of the terminal. */
    void clear();

    EVETerminalProcessor& getTerminalProcessor() const;

    /**
     * @brief Sets the visibility of the side preview from a path. A non-empty path shows the side preview, and an empty path hides it.
     * @param path  Path of the image file.
     */
    void setPreview (const juce::String& path);

    /**
     * @brief Writes the visibility of the side preview to the model.
     * @param isVisible  `true` to show the side preview.
     */
    void setPreviewVisible (bool isVisible);

    /** @brief Returns `true` when the model says that the side preview is visible. */
    bool isPreviewVisible() const;

    void invokeCommand (const juce::Identifier& name);

    /**
     * @brief Adds the handlers of the `terminal` tree properties to `terminalEvents`.
     *
     * The `commandCount` handler runs the commands of the command FIFO. The `preview` handler sets the side preview from the path.
     * The `clipboard` handler copies the image text to the system clipboard. Each report property gets a handler that forwards its text through `applyTerminalReport()`.
     * In a profile build, the `promptCount` handler repaints the view when the count is 1, and it reports `Id::promptPainted` to the processor.
     */
    void registerTerminalEvents();

    /**
     * @brief Sends the name and the text of a report property to the processor.
     *
     * The text goes through the `Id::terminalReport` setter of the processor.
     *
     * @param terminal  The `terminal` tree that holds the property.
     * @param id        Report property that changed.
     */
    void applyTerminalReport (const juce::ValueTree& terminal, const juce::Identifier& id);

    /** Shared instance of the `map::CommandId` map. */
    jam::SharedInstance<map::CommandId> commandId { std::in_place };

    /** Overlay that shows the settings message. It shows the message when the message callback runs with a non-empty MESSAGE component property. */
    jam::MessageOverlay messageOverlay;

    jam::Function::Map<juce::Identifier, void> terminalEvents;    ///< Handlers of the `terminal` tree properties, keyed by property identifier. `valueTreePropertyChanged()` runs them.

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EVEView)
};
