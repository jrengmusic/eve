#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"

/**
 * @class EVETerminalProcessor
 * @brief Terminal chain of the plugin. It presents the rows of the engine ring to the `jam::TerminalProcessor`, which owns the live document and the history record.
 *
 * `process()` runs the terminal processor. The engine writes the rows that leave the live area to the history FIFO,
 * and it writes a changed live area to the live FIFO. The terminal processor owns the history `jam::SwapDocument`.
 * The view pulls through the getter in `userInterfaceGetters`: the history bytes append to the history
 * record, and the view reads the record and the live document inside the call and stores neither.
 * The view sends input bytes through the `terminalInput` event. The bytes wait in `pendingInput` until the input lane has room.
 *
 * @par Thread contract
 * `process()` runs on the audio thread. It writes the input lane to the TTY, then decodes the TTY output.
 * `setWinsize()` runs on the message thread. When the cell grid of the model winsize differs from the grid of the engine,
 * it calls `prepare()` between `suspendProcessing (true)` and `suspendProcessing (false)`, so `process()` does not run during the change.
 * `prepare()` runs on the message thread.
 */
class EVETerminalProcessor : public juce::AudioProcessorValueTreeState::Listener,
                             private jam::AsyncUpdater
{
public:
    /**
     * @brief Sizes the input lane, registers the parameter setters, the events, the user interface getters and the chain events, then listens to the model.
     * @param processorToUse  Processor that owns this object. The object suspends it while `setWinsize()` prepares.
     * @param modelToUse      Model that holds the winsize and the parameters. It must outlive this object.
     */
    EVETerminalProcessor (juce::AudioProcessor& processorToUse, jam::AudioModel& modelToUse);

    ~EVETerminalProcessor() override;

    /**
     * @brief Prepares the engine and the terminal processor from a process spec and the model winsize, and sizes or opens the TTY.
     *
     * The unnamed parameter is the process spec. A sample rate or a maximum block size of zero falls back to the value of the processor.
     * The terminal processor gets the number of blocks of one vblank. That number is the sample rate divided by the block size
     * and by `jam::VulkanEngine::getRefreshRateHz()`, rounded up, and at least 1.
     *
     * When the model winsize is valid and its cell grid differs from the grid of the engine, the function prepares the engine
     * and then the terminal processor, and it resets the screen. The terminal processor first drains the history FIFO into the history record.
     * The TTY takes the new size when it runs. Otherwise the function opens it with the shell of the `SHELL` variable.
     * The history FIFO size does not limit the history record.
     *
     * When the grid is the same, the function prepares only the terminal processor. It does so only when the sample rate is above zero
     * and the grid of the engine is not empty.
     */
    void prepare (const juce::dsp::ProcessSpec&);

    /** @brief Audio thread. Unless bypassed, it writes the input lane to the TTY, then the terminal processor reads and decodes the TTY output and publishes the frame. The live area has the row count of the current window size. */
    void process() noexcept;

    /** @brief Prepares again from the model winsize, under `suspendProcessing`. `prepare()` changes the engine only when the cell grid differs. */
    void setWinsize();

    int writeSwap();

    std::function<void()> onProcessRequest;

    /**
     * @brief Stores the bypass state when the bypass parameter changes.
     * @param parameterID  ID of the parameter that changed; only the bypass parameter has a setter.
     * @param newValue     New parameter value; a non-zero value bypasses.
     */
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    jam::ViewManager::Getters userInterfaceGetters;     ///< Getters for the view. The constructor adds the text editor getter. It writes the pending input, pulls the terminal processor, then gives the history record, the live document, and whether the pull replaced the live document to a `jam::TerminalProcessor::Visit`.
    jam::ViewManager::Events chainEvents;               ///< Events of the chain. The constructor adds the `terminalInput` event. It appends its bytes to the pending input, then writes the bytes that the input lane has room for.

private:
    void handleAsyncUpdate() override;

    /** @brief Adds the bypass setter to the parameter setters. */
    void registerParameterSetters();

    /** @brief Adds the event setters that copy the terminal state to the model parameters. */
    void registerEvents();

    /** @brief Adds the text editor getter keyed by the winsize type. It writes the pending input, pulls the terminal processor, then calls the visitor with the history record, the live document, and whether the pull replaced the live document. */
    void registerUserInterfaceGetters();

    /** @brief Adds the `terminalInput` event to the chain events. It appends its bytes to `pendingInput`, then calls `writePendingInput()`. */
    void registerChainEvents();

    /**
     * @brief Sets a model parameter to a value and tells the host, only when the value differs.
     * @param parameterID  ID of the parameter.
     * @param value        Value in the range of the parameter.
     */
    void setParameterValue (juce::StringRef parameterID, float value);

    /** @brief Writes the bytes of `pendingInput` that the input lane has room for, publishes the lane, and removes the written bytes from `pendingInput`. */
    void writePendingInput();

    int getHistoryCapacity() const;

    /** @brief Returns the winsize that the model holds. */
    jam::TerminalWinsize getWinsize() const;

    /** @brief Returns the TTY of the platform. */
    static std::unique_ptr<jam::TerminalTTY> createTTY();

    juce::AudioProcessor& processor;                  ///< Processor that `setWinsize()` suspends.
    jam::AudioModel& model;                           ///< Model that holds the winsize and the parameters.
    std::atomic<bool> isBypassed { false };           ///< Bypass state. `parameterChanged()` writes it, and `process()` reads it.
    jam::Function::Map<juce::String, void> parameters;   ///< Parameter setters, keyed by parameter ID.
    jam::Function::Map<juce::Identifier, void> events;   ///< Event setters that copy the terminal state to the model parameters, keyed by event ID.
    jam::TerminalFIFO input;                          ///< Input lane from the view to the TTY.
    juce::MemoryBlock pendingInput;                   ///< Input bytes that the input lane had no room for yet, in order.
    std::unique_ptr<jam::TerminalTTY> tty { createTTY() };   ///< Pseudo terminal of the shell.
    jam::TerminalProcessor terminalProcessor;           ///< History FIFO, live FIFO, the live document and the history record.

    static constexpr bool shouldUseInlineImages { true };   ///< Whether the engine presents image placements as inline images.

    jam::TerminalGraphicsEngine engine { shouldUseInlineImages };     ///< Row grid that `process()` presents.
    jam::TerminalSwap swap;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EVETerminalProcessor)
};
