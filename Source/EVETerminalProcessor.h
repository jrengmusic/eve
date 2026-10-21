#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"

/**
 * @class EVETerminalProcessor
 * @brief Terminal chain of the plugin: renders a markdown harness into a terminal row grid and carries the wire bytes to the text editor.
 *
 * `load()` paints a markdown file into the graphics engine. `process()` makes the engine write the
 * rows to the history FIFO and the live FIFO of the terminal processor. The text editor of the view
 * pulls the bytes through the getter in `userInterfaceGetters`.
 *
 * @par Thread contract
 * `setWinsize()` stores the size atomically, and `process()` reads it. `load()`, `prepare()` and `reset()` run on the message thread.
 */
class EVETerminalProcessor
{
public:
    /**
     * @brief Registers the user interface getters.
     * @param processorToUse  Audio processor that `load()` suspends while it paints. It must outlive this object.
     * @param modelToUse      Model that gives the history capacity and the appearance. It must outlive this object.
     */
    EVETerminalProcessor (juce::AudioProcessor& processorToUse, jam::AudioModel& modelToUse);

    /**
     * @brief Sizes both FIFOs of the terminal processor.
     *
     * The live FIFO holds the bytes of one frame of the current window size. The history FIFO holds
     * the live bytes plus the bytes of a grid as wide as the engine and as tall as the history capacity of the model (jassert > 0).
     *
     * @param newSpec  Processing spec; the processor keeps it for `load()`.
     */
    void prepare (const juce::dsp::ProcessSpec& newSpec);

    /** @brief Discards the bytes in both FIFOs. */
    void reset() noexcept;

    /** @brief Makes the engine write its ready rows to the FIFOs. The live area has the row count of the current window size. */
    void process() noexcept;

    /**
     * @brief Stores the window size that `prepare()` and `process()` read.
     * @param newWinsize  Size of the terminal view, in cells and in pixels.
     */
    void setWinsize (jam::TerminalWinsize newWinsize);

    /**
     * @brief Renders a markdown file into the graphics engine.
     *
     * The function maps the file, parses it as markdown and lays it out in a markdown component whose width is the column width of
     * the style. It suspends the audio processor, prepares the engine for that grid, paints the component through a terminal
     * graphics context, calls `prepare()` with the stored spec and resumes the audio processor.
     *
     * @param harness  Markdown file to render (jassert that it maps).
     */
    void load (const juce::File& harness);

    jam::ViewManager::Getters userInterfaceGetters;     ///< Getters for the view. The constructor adds the text editor getter that pulls the bytes of both FIFOs.
    jam::ViewManager::Events chainEvents;               ///< Events of the chain. The class adds none.

private:
    /** @brief Adds the text editor getter keyed by the winsize type. It calls `jam::TerminalProcessor::pull()`. */
    void registerUserInterfaceGetters();

    juce::AudioProcessor& processor;                    ///< Audio processor that `load()` suspends.
    jam::AudioModel& model;                             ///< Model that gives the history capacity and the appearance.
    juce::dsp::ProcessSpec spec { 44100.0, 1024, 2 };   ///< Spec of the last `prepare()`.
    std::atomic<int64_t> pendingWinsize { 0 };          ///< Packed `jam::TerminalWinsize` that `setWinsize()` stores.
    jam::TerminalProcessor terminalProcessor;           ///< History FIFO and live FIFO.

    static constexpr bool shouldUseInlineImages { true };   ///< Whether the engine presents image placements as inline images.

    jam::terminal::GraphicsEngine engine { shouldUseInlineImages };     ///< Row grid that `load()` paints and `process()` presents.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EVETerminalProcessor)
};
