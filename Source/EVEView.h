#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"

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
     * A change of `Id::commandCount` runs the commands in the command FIFO. A change of `Id::preview` sets the visibility of the side preview:
     * a non-empty path shows it. The base class gets every change.
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

    /** @brief Shows the message that the history cleared, clears the terminal, and restarts the terminal processor with the new history capacity. */
    void setHistoryCapacity();

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

    /**
     * @brief Runs each command that the shell wrote to the command FIFO.
     *
     * The names are terminated by a null byte. A name that spans two pending blocks is joined before it is matched.
     * A name that the command manager has no command for is ignored.
     */
    void applyCommands();

    /** Shared instance of the `map::CommandId` map. */
    jam::SharedInstance<map::CommandId> commandId { std::in_place };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EVEView)
};
