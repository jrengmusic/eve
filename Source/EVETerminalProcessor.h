#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"

/**
 * @class EVETerminalProcessor
 * @brief Terminal chain of the plugin. It decodes the TTY output into the rows of a `jam::TerminalBuffer` and publishes them through a `jam::TerminalSwap`.
 *
 * `process()` runs the `jam::TerminalProcessor`. The processor decodes the TTY output into the rows of the buffer.
 * A row that leaves the live area stays in the ring of the buffer as a history row. `writePendingData()` moves the history rows into the swap.
 * The processor writes a changed live area to the swap on each `process()` call.
 * The view pulls through the getter in `userInterfaceGetters`. The getter gives the swap, and the view reads the history lines and the live lines from it.
 * The view sends input bytes through the `terminalInput` event. The bytes wait in `pendingInput` until the input lane has room.
 *
 * @par Thread contract
 * `process()` runs on the audio thread. It writes the input lane to the TTY, then decodes the TTY output.
 * `setWinsize()` runs on the message thread. When the cell grid of the model winsize differs from the grid of the buffer,
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
     * @brief Sizes the input lane and the decode buffers from a process spec, then prepares the buffer, the swap and the TTY from the model winsize.
     *
     * The unnamed parameter is the process spec. Its maximum block size (jassert > 0) sizes the input lane and the buffers of the terminal processor.
     * The block size and the sample rate for the ring of the buffer come from the processor, not from the spec.
     * The ring of the buffer holds the history rows in transit: one row for each byte of the blocks of one vblank, that is the number of blocks times the block size.
     * The number of blocks is the sample rate divided by the block size and by `jam::VulkanEngine::getRefreshRateHz()`, rounded up, and at least 1.
     *
     * When the model winsize is valid and the block size of the processor is above zero, the function writes the pending history rows to the swap.
     * It then prepares the buffer and the swap, resets the screen of the terminal processor, and sets the history capacity of the swap.
     * When the TTY runs, the TTY takes the new size. Otherwise the function opens the TTY with the shell of the `SHELL` variable and the block size of the processor.
     */
    void prepare (const juce::dsp::ProcessSpec&);

    /** @brief Audio thread. Unless bypassed, it writes the input lane to the TTY, then the terminal processor reads and decodes the TTY output until a read returns no byte. Each call of the terminal processor writes the changed live area to the swap. The live area has the row count of the current window size. */
    void process() noexcept;

    /** @brief Prepares again from the model winsize, under `suspendProcessing`. The call prepares the buffer, the swap and the TTY only when the cell grid differs. */
    void setWinsize();

    /** @brief Writes the pending history rows to the swap, then blanks the screen and the saved lines under `suspendProcessing`, and asks for a process. */
    void clear();

    /** @brief Prepares again from the model winsize and the model history capacity, under `suspendProcessing`. */
    void setHistoryCapacity();

    /**
     * @brief Moves the history rows of the buffer into the swap.
     * @return `true` when the call wrote at least one line.
     */
    bool writePendingData();

    std::function<void()> onProcessRequest;

    /**
     * @brief Stores the bypass state when the bypass parameter changes.
     * @param parameterID  ID of the parameter that changed; only the bypass parameter has a setter.
     * @param newValue     New parameter value; a non-zero value bypasses.
     */
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    jam::ViewManager::Getters userInterfaceGetters;     ///< Getters for the view. The constructor adds the text editor getter. It writes the pending input, asks for the pending history rows, then writes a pointer to the swap into its output argument.
    jam::ViewManager::Events chainEvents;               ///< Events of the chain. The constructor adds the `terminalInput` event. It appends its bytes to the pending input, then writes the bytes that the input lane has room for.

private:
    /** @brief Writes the pending history rows to the swap, and asks for a process when it wrote a line. */
    void handleAsyncUpdate() override;

    /** @brief Adds the bypass setter to the parameter setters. */
    void registerParameterSetters();

    /** @brief Adds the event setters that copy the terminal state to the model parameters. */
    void registerEvents();

    /** @brief Adds the text editor getter keyed by the winsize type. It writes the pending input and asks for the pending history rows: in the standalone wrapper through an async update, else at once. It then writes a pointer to the swap into its output argument and asks for a process. */
    void registerUserInterfaceGetters();

    /** @brief Adds the `terminalInput` event to the chain events. It appends its bytes to `pendingInput`, then calls `writePendingInput()`. */
    void registerChainEvents();

    /**
     * @brief Sets a model parameter to a value and tells the host, only when the value differs.
     * @param parameter  Parameter to set.
     * @param value      Value in the range of the parameter.
     */
    void setParameterValue (juce::RangedAudioParameter& parameter, float value);

    /** @brief Writes the bytes of `pendingInput` that the input lane has room for, publishes the lane, and removes the written bytes from `pendingInput`. */
    void writePendingInput();

    int getHistoryCapacity() const;

    bool isShellIntegrated() const;

    juce::File getStartDirectory() const;

    static void addShellScripts (const juce::File& directory);

    static juce::StringPairArray getShellEnvironment (const juce::String& shell, const juce::File& directory);

    static juce::String getShellArguments (const juce::String& shell, const juce::File& directory);

    /** @brief Returns the winsize that the model holds. */
    jam::TerminalWinsize getWinsize() const;

    void prepare (jam::TerminalWinsize winsize, int historyCapacity);

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
    jam::TerminalProcessor terminalProcessor;           ///< Decoder of the TTY output into the rows of `terminalBuffer`.

    static constexpr bool shouldUseInlineImages { true };   ///< Whether the engine presents image placements as inline images.

    jam::TerminalBuffer terminalBuffer { shouldUseInlineImages };     ///< Row grid that the terminal processor writes. Its ring holds the history rows until `writePendingData()` moves them.
    jam::TerminalSwap swap;                           ///< Store of the history lines, the live area and the true colours. The view reads it.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EVETerminalProcessor)
};
