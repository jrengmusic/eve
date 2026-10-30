#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"

/**
 * @class EVEView
 * @brief Plugin editor of EVE: the terminal view, a side preview, and one overlay for each place that shows an inline image.
 *
 * The view shows the `jam::TextEditor` of the terminal. The editor reserves rows for the inline images and reports their boxes.
 * The view puts a `jam::Preview` over each box and draws the image in it. The bytes come from the swap of the terminal processor.
 * The side preview shows the image file that the `preview` command names. It takes the right part of the editor while it is visible.
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
     * @brief Builds the editor, scales it, and asks for the keyboard focus.
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
     * A change of `Id::commandCount` runs the commands in the command FIFO. A change of `Id::preview` shows the file that the property names in the side preview.
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

    /** @brief Builds the registry with the buttons and the view components. */
    void initialiseRegistry() override;

    /** @brief Builds the view from the view layout and the getters and events of the terminal processor, then adds the view and the side preview. */
    void initialiseView() override;

    /**
     * @brief Registers the buttons.
     * @param registration  Registration that receives the buttons.
     */
    void registerButtons (jam::Registry::Registration& registration);

    /**
     * @brief Registers the view components, their events, their styles and the content of the settings.
     *
     * The event of the text editor binds the keys, the mouse, the image boxes and the live caret values of the editor to the terminal.
     *
     * @param registration  Registration that receives the view components.
     */
    void registerViewComponents (jam::Registry::Registration& registration);

    /** @brief Adds the callbacks of the panels: scale, window size, clear, preview and history capacity. */
    void registerCallbacks();

    /** @brief Clears the screen and the saved lines of the terminal. */
    void clear();

    /** @brief Shows the message that the history cleared, clears the terminal, and restarts the terminal processor with the new history capacity. */
    void setHistoryCapacity();

    /**
     * @brief Shows an image file in the side preview. An empty path hides the side preview.
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
     * @brief Matches the overlays to the boxes of the images in the text editor.
     *
     * Each box has one overlay, in the bounds of the box. A new box gets a new overlay. An overlay whose box is gone is removed.
     *
     * @param textEditor   Text editor that reported the boxes.
     * @param imageBounds  Boxes of the visible images, keyed by image number, in the space of @p textEditor.
     */
    void setOverlays (const juce::Component& textEditor, const jam::HashMap<juce::int64, jam::Array<juce::Rectangle<int>>>& imageBounds);

    /** @brief Runs each command that the shell wrote to the command FIFO. A name that is not a command is ignored. */
    void applyCommands();

    /**
     * @brief Adds an overlay that shows an image.
     *
     * The call adds nothing when the swap holds no bytes for the image.
     *
     * @param image     Number of the image in the record.
     * @param previews  Overlays of the image. The new overlay joins them.
     */
    void applyImage (juce::int64 image, jam::Owner<jam::Preview>& previews);

    jam::HashMap<juce::int64, jam::Owner<jam::Preview>> overlays;   ///< Overlays over the text editor, keyed by image number. Each box of an image has one.

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EVEView)
};
