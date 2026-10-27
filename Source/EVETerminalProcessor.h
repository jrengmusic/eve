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
 * @par Model lane
 * The object owns a terminal `jam::Model`, `state`. Its tree is the `terminal` child of the APVTS state of the model.
 * `jam::ModelLayout` adds the parameters of the `modelLayout` table to it.
 * On each clock of the terminal processor, the events call `storeValue` on `state`. They store the terminal modes and the cursor.
 * The Model does not run its own timer. The getter in `userInterfaceGetters` flushes it on each vblank, and `flush()` flushes it for the host.
 *
 * @par Thread contract
 * `process()` runs on the audio thread. It writes the input lane to the TTY, then decodes the TTY output.
 * `prepare()` runs on the message thread. `EVEProcessor::prepareToPlay` calls it, and the processor suspends processing around the call.
 */
class EVETerminalProcessor : private jam::AsyncUpdater
{
public:
    /**
     * @brief Builds the Model lane, registers the events, the user interface getters and the chain events, then forwards the TTY readiness to `onBytesReady`.
     *
     * The constructor binds `bypass` to the atomic of the bypass parameter in the model.
     * It builds `state` on the `terminal` child of the model state, stops the timer of `state`, and adds the parameters of the `modelLayout` table to it.
     *
     * @param processorToUse  Processor that owns this object. The object reads its wrapper type, and `clear()` suspends it.
     * @param modelToUse      Model that holds the winsize, the bypass parameter and the history capacity. It must outlive this object.
     */
    EVETerminalProcessor (juce::AudioProcessor& processorToUse, jam::AudioModel& modelToUse);

    ~EVETerminalProcessor() override;

    /**
     * @brief Sizes the input lane and the decode buffers from a process spec, then prepares the buffer, the swap and the TTY from the model winsize.
     *
     * The parameter `spec` is the process spec. Its maximum block size (jassert > 0) sizes the input lane and the buffers of the terminal processor.
     * The block size and the sample rate of the spec size the ring of the buffer.
     * The ring of the buffer holds the history rows in transit: one row for each byte of the blocks of one vblank, that is the number of blocks times the block size.
     * The number of blocks is the sample rate divided by the block size and by `jam::VulkanEngine::getRefreshRateHz()`, rounded up, and at least 1.
     *
     * When the model winsize is valid, the function writes the pending history rows to the swap.
     * It then prepares the buffer and the swap, resets the terminal processor, and sets the history capacity of the swap.
     * The TTY opens on the first call. Later calls only resize the TTY.
     *
     * @param spec     Process spec: sample rate and maximum block size.
     * @param context  Serial context for the TTY readiness source. The TTY runs `onBytesReady` on it.
     */
    void prepare (const juce::dsp::ProcessSpec&, jam::AsyncUpdater::Context context);

    /** @brief Audio thread. It reads `bypass`. When `bypass` is 0, it writes the input lane to the TTY, then the terminal processor reads and decodes the TTY output until a read returns no byte. Each call of the terminal processor writes the changed live area to the swap. The live area has the row count of the current window size. */
    void process() noexcept;

    /** @brief Writes the pending history rows to the swap, then blanks the screen and the saved lines under `suspendProcessing`, and asks for a process. */
    void clear();

    /**
     * @brief Moves the history rows of the buffer into the swap.
     * @return `true` when the call wrote at least one line.
     */
    bool writePendingData();

    /** @brief Called when a process pass is due: after the input lane took bytes, after a history write, after the user interface getter and after `clear()`. The owner sets it. */
    std::function<void()> onProcessRequest;

    /** @brief Called when the TTY has bytes to read. The TTY runs it on the serial context that `prepare()` received. The owner sets it. */
    std::function<void()> onBytesReady;

    /**
     * @brief Joins the Model lane to a restored state tree.
     *
     * When @p newState has a `terminal` child, `state` replaces its content with that child, and the child leaves @p newState.
     * The tree of `state` then moves from its parent to @p newState.
     *
     * @param newState  State tree that the host restored. It receives the tree of `state`.
     */
    void setState (juce::ValueTree newState);

    /** @brief Takes the message manager lock, then flushes the Model lane to its tree. */
    void flush();

    jam::ViewManager::Getters userInterfaceGetters;     ///< Getters for the view. The constructor adds the text editor getter. It writes the pending input, asks for the pending history rows, flushes the Model lane, then writes a pointer to the swap into its output argument.
    jam::ViewManager::Events chainEvents;               ///< Events of the chain. The constructor adds the `terminalInput` event. It appends its bytes to the pending input, then writes the bytes that the input lane has room for.

private:
    /** @brief Writes the pending history rows to the swap, and asks for a process when it wrote a line. */
    void handleAsyncUpdate() override;

    /** @brief Adds the event setters that copy the terminal state to the Model lane. Each setter calls `storeValue` on `state`. The mode events store a bool as 0 or 1, and `mouseTracking` and `cursor` store an int. */
    void registerEvents();

    /** @brief Adds the text editor getter keyed by the winsize type. It writes the pending input and asks for the pending history rows: in the standalone wrapper through an async update, else at once. It then flushes the Model lane, writes a pointer to the swap into its output argument and asks for a process. */
    void registerUserInterfaceGetters();

    /** @brief Adds the `terminalInput` event to the chain events. It appends its bytes to `pendingInput`, then calls `writePendingInput()`. */
    void registerChainEvents();

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

    /**
     * @brief Prepares the buffer, the swap and the terminal processor for a winsize, then opens or resizes the TTY.
     * @param spec             Process spec. Its block size and sample rate size the ring of the buffer.
     * @param winsize          Grid size in cells and pixels.
     * @param historyCapacity  Number of history lines that the swap keeps.
     * @param context          Serial context that the TTY receives in `open()`.
     */
    void prepare (const juce::dsp::ProcessSpec& spec, jam::TerminalWinsize winsize, int historyCapacity, jam::AsyncUpdater::Context context);

    /** @brief Returns the TTY of the platform. */
    static std::unique_ptr<jam::TerminalTTY> createTTY();

    juce::AudioProcessor& processor;                  ///< Processor that owns this object. The object reads its wrapper type, and `clear()` suspends it.
    jam::AudioModel& model;                           ///< Model that holds the winsize and the parameters.
    const std::atomic<float>& bypass;                 ///< Atomic of the bypass parameter in the model. `process()` runs only while it is 0.
    jam::Model state;                                 ///< Terminal Model. Its tree is the `terminal` child of the model state. The events store the terminal state into it, and the flush writes it to the tree.
    jam::Function::Map<juce::Identifier, void> events;   ///< Event setters that copy the terminal state to the Model lane, keyed by event ID.
    jam::TerminalFIFO input;                          ///< Input lane from the view to the TTY.
    juce::MemoryBlock pendingInput;                   ///< Input bytes that the input lane had no room for yet, in order.
    std::unique_ptr<jam::TerminalTTY> tty { createTTY() };   ///< Pseudo terminal of the shell.
    jam::TerminalProcessor terminalProcessor;           ///< Decoder of the TTY output into the rows of `terminalBuffer`.

    static constexpr bool shouldUseInlineImages { true };   ///< Whether the engine presents image placements as inline images.

    jam::TerminalBuffer terminalBuffer { shouldUseInlineImages };     ///< Row grid that the terminal processor writes. Its ring holds the history rows until `writePendingData()` moves them.
    jam::TerminalSwap swap;                           ///< Store of the history lines, the live area and the true colours. The view reads it.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EVETerminalProcessor)
};
