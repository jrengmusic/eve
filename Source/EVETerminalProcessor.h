#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"

/**
 * @class EVETerminalProcessor
 * @brief Terminal chain of the plugin. It presents the rows of the engine ring to the `jam::TerminalProcessor`, which owns the live document.
 *
 * `process()` makes the engine write the
 * rows to the history FIFO and the live FIFO of the terminal processor. The view owns the history document.
 * The view pulls through the getter in `userInterfaceGetters`: the history bytes append to the history
 * document of the view, and the view reads the live document inside the call and stores nothing.
 *
 * @par Thread contract
 * `setWinsize()` stores the size atomically, and `process()` reads it. `prepare()` and `reset()` run on the message thread.
 */
class EVETerminalProcessor
{
public:
    /**
     * @brief Registers the user interface getters.
     * @param modelToUse  Model that holds the history capacity setting. It must outlive this object.
     */
    explicit EVETerminalProcessor (jam::AudioModel& modelToUse);

    /**
     * @brief Sizes both FIFOs of the terminal processor.
     *
     * The live FIFO holds the bytes of one frame of the current window size. The history FIFO holds
     * the live bytes, the bytes of the rows that the engine holds now, and the bytes of a grid as wide as the
     * current window and as tall as the history capacity of the model (jassert > 0).
     */
    void prepare (const juce::dsp::ProcessSpec&);

    /** @brief Discards the bytes in both FIFOs. */
    void reset() noexcept;

    /** @brief Makes the engine write its ready rows to the FIFOs. The live area has the row count of the current window size. */
    void process() noexcept;

    /**
     * @brief Stores the window size that `prepare()` and `process()` read.
     * @param newWinsize  Size of the terminal view, in cells and in pixels.
     */
    void setWinsize (jam::TerminalWinsize newWinsize);

    jam::ViewManager::Getters userInterfaceGetters;     ///< Getters for the view. The constructor adds the text editor getter. It pulls the history bytes into the history document of the view and gives the live document, whether the pull replaced it, and the history capacity to a visitor.
    jam::ViewManager::Events chainEvents;               ///< Events of the chain. The class adds none.

private:
    /** @brief Adds the text editor getter keyed by the winsize type. It pulls into the history document of the view, then calls the visitor with the live document, whether the pull replaced it, and the history capacity from the settings. */
    void registerUserInterfaceGetters();

    /** @brief Returns the history capacity setting from the current state tree. */
    int getHistoryCapacity() const;

    jam::AudioModel& model;                             ///< Model that holds the history capacity setting.
    std::atomic<int64_t> pendingWinsize { 0 };          ///< Packed `jam::TerminalWinsize` that `setWinsize()` stores.
    jam::TerminalProcessor terminalProcessor;           ///< History FIFO, live FIFO, and the live document.

    static constexpr bool shouldUseInlineImages { true };   ///< Whether the engine presents image placements as inline images.

    jam::TerminalGraphicsEngine engine { shouldUseInlineImages };     ///< Row grid that `process()` presents.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EVETerminalProcessor)
};
