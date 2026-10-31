#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"

/**
 * @class EVETerminalProcessor
 * @brief Terminal chain of the plugin. It decodes the TTY output into the rows of a `jam::TerminalBuffer` and publishes them through a `jam::TerminalSwap`.
 *
 * `process()` runs the `jam::TerminalProcessor`. The processor decodes the TTY output into the rows of the buffer.
 * A row that leaves the live area stays in the ring of the buffer as a history row. `writePendingData()` moves the history rows into the swap,
 * and it moves the image records of the processor into the swap with them.
 * The processor writes a changed live area to the swap on each `process()` call.
 * The processor does not decode images. The view reads the bytes of an image from the swap and decodes them.
 * The view pulls through the getter in `userInterfaceGetters`. The getter gives the swap, and the view reads the history lines and the live lines from it.
 * The view sends input bytes through the `terminalInput` event. The event writes the bytes into the input lane when the lane has room for them, and asks for a process.
 *
 * @par Model lane
 * The object owns a terminal `jam::Model`, `state`. Its tree is the `terminal` child of the APVTS state of the model.
 * `jam::ModelLayout` adds the parameters of the `model` table in `parametersLayout` to it.
 * On each clock of the terminal processor, the events call `storeValue` on `state`. They store the terminal modes and the cursor.
 * The Model does not run its own timer. The getter in `userInterfaceGetters` flushes it on each vblank, and `flush()` flushes it for the host.
 *
 * @par Clock
 * The object owns its clock, `jam::VirtualDevice device`. The object is the `juce::AudioIODeviceCallback` of the clock.
 * The tick, `audioDeviceIOCallbackWithContext`, runs `process()` on the serial context of the device.
 * The TTY readiness source runs on the same context. Its callback calls `device.clock (numSamples)`.
 * `numSamples` is the number of bytes that the TTY reports as ready. The TTY waits its period between two ticks.
 *
 * @par Thread contract
 * `process()` runs on the serial context of the device. It writes the input lane to the TTY, then decodes the TTY output.
 * `prepare()`, `releaseResources()`, `restart()` and `clear()` run on the message thread.
 * The winsize attachment of `state` calls `restart()` when the winsize changes. A stopped clock stays stopped, so a winsize change prepares the chain only while the clock runs.
 * `EVEProcessor::prepareToPlay` calls `prepare()`. `EVEProcessor::releaseResources` calls `releaseResources()`.
 */
class EVETerminalProcessor
    : private jam::AsyncUpdater
    , private juce::ValueTree::Listener
    , private juce::AudioIODeviceCallback
{
public:
    /**
     * @brief Builds the Model lane, registers the events, the user interface getters and the chain events, then sets the TTY readiness to clock the device.
     *
     * The constructor binds `bypass` to the atomic of the bypass parameter in the model.
     * It builds `state` on the `terminal` child of the model state, stops the timer of `state`, and adds the parameters of the `model` table in `parametersLayout` to it.
     *
     * @param processorToUse  Processor that owns this object. The object reads its wrapper type in profile builds.
     * @param modelToUse      Model that holds the winsize, the bypass parameter and the history capacity. It must outlive this object.
     */
    EVETerminalProcessor (juce::AudioProcessor& processorToUse, jam::AudioModel& modelToUse);

    /** @brief Stops listening to `state` and cancels a pending update. The clock must be stopped (jassert). */
    ~EVETerminalProcessor() override;

    /**
     * @brief Stops the clock, opens it at the fixed sample rate and the device buffer size of the OS, then starts it unconditionally.
     *
     * The device buffer size is `virtualBlockSize` on Windows and 1024 on the other systems.
     * The start runs `audioDeviceAboutToStart`, which prepares the chain.
     */
    void prepare();

    /** @brief Stops the clock, then closes the device. `stop` waits for a tick in progress. */
    void releaseResources();

    /**
     * @brief Stops the clock, calls @p callback, then starts the clock again only when it ran before the call.
     *
     * A stopped clock stays stopped.
     *
     * @tparam Callback  Callable type with no argument.
     * @param callback   Callable that runs while the clock is stopped.
     */
    template<typename Callback>
    void restart (Callback&& callback)
    {
        const auto isPlaying { device.isPlaying() };

        device.stop();
        callback();

        if (isPlaying)
            start();
    }

    /** @brief Restarts with no change, so the chain prepares again from the Model winsize and history capacity. */
    void restart();

    /** @brief Inside `restart`, writes the pending history rows to the swap, then blanks the screen and the saved lines. */
    void clear();

    /**
     * @brief Moves the history rows of the buffer and the image records of the processor into the swap.
     * @return `true` when the call wrote at least one line, or when image records were waiting.
     */
    bool writePendingData();

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

    /** @brief Returns the FIFO of the command names that the shell sent. Each name ends with a zero byte. The view reads it. */
    jam::TerminalFIFO& getCommands() noexcept;

    /** @brief Returns the swap that holds the history lines, the live area and the image bytes. The view reads it. */
    jam::TerminalSwap& getSwap() noexcept;

#if JAM_USING_PROFILER
    /**
     * @brief Returns the manifest file of a profile build.
     * @return File that the first command-line argument names, resolved against the working folder.
     */
    static juce::File getManifestFile();
#endif

    jam::ViewManager::Getters
        userInterfaceGetters;///< Getters for the view. The constructor adds the text editor getter. It posts the history writer to the serial context of this object, flushes the Model lane, gives the swap, and asks for a process.
    jam::ViewManager::Events
        chainEvents;///< Events of the chain. The constructor adds the `terminalInput` event. It writes its bytes into the input lane when the lane has room for them, then asks for a process.

private:
    /** @brief Starts the clock with this object as the callback, and asks for a process. */
    void start();

    /** @brief Runs on the tick of the clock. It reads `bypass`. When `bypass` is 0, it writes the input lane to the TTY, then the terminal processor reads and decodes the TTY output until a read does not fill its read space. Each call of the terminal processor writes the changed live area to the swap. The live area has the row count of the current window size. */
    void process() noexcept;

    /** @brief The tick of the clock. It runs `process()`. */
    void audioDeviceIOCallbackWithContext (const float* const*,
                                           int,
                                           float* const*,
                                           int,
                                           int,
                                           const juce::AudioIODeviceCallbackContext&) override;

    /** @brief Sizes the input lane, the command FIFO and the decoder with `virtualBlockSize`. When the winsize is valid, it prepares the buffer, the swap and the TTY. */
    void audioDeviceAboutToStart (juce::AudioIODevice*) override;

    /** @brief Does nothing. */
    void audioDeviceStopped() override;

    /** @brief Writes the pending history rows to the swap, and asks for a process when it wrote a line. */
    void handleAsyncUpdate() override;

    /**
     * @brief Quits the application when the shell exits.
     *
     * The call quits when @p property is `shellExited` and its flushed value is true.
     *
     * @param tree      Tree that changed.
     * @param property  Property that changed.
     */
    void
    valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property) override;

    /**
     * @brief Adds the event setters that copy the terminal state to the Model lane.
     *
     * Each setter calls `storeValue` on `state`. The mode events and `shellExited` store a bool as 0 or 1, and `mouseTracking` and `cursor` store an int.
     * The `preview` event stores the path text. The `previewImage` event releases the previous preview image in the buffer, then stores the number of the new image.
     * The `command` event adds a command name through `addCommand()`.
     */
    void registerEvents();

    /**
     * @brief Writes a command name and a zero byte to the command FIFO, and counts the command in the Model lane.
     *
     * The command FIFO must have room for the name and the zero byte (jassert). The call writes nothing when the name is empty or does not fit.
     *
     * @param name  Bytes of the command name.
     * @param size  Number of bytes in @p name.
     */
    void addCommand (const char* name, int size);

    /** @brief Adds the text editor getter keyed by the winsize type. The getter posts the history writer to the serial context of this object, flushes the Model lane, gives the swap, and asks for a process. */
    void registerUserInterfaceGetters();

    /** @brief Adds the `terminalInput` event to the chain events. It writes its bytes into the input lane when the lane has room for them, then asks for a process. */
    void registerChainEvents();

    /** @brief Returns the history capacity that the `historyCapacity` component of the model holds. */
    int getHistoryCapacity() const;

    /** @brief Returns the winsize that the model holds. */
    jam::TerminalWinsize getWinsize() const;

    /**
     * @brief Prepares the buffer, the swap, the decoder and the TTY for a window size.
     *
     * The call first writes the pending history rows to the swap.
     * It sizes the buffer for the blocks of one vblank, prepares the swap, resets the decoder, and sets the history capacity.
     * It then opens the TTY and sets its window size.
     *
     * @param winsize          Window size of the grid.
     * @param historyCapacity  Number of history lines that the swap keeps.
     */
    void prepare (jam::TerminalWinsize winsize, int historyCapacity);

    /**
     * @brief Opens the TTY on the serial context of the device.
     *
     * In a profile standalone build, the shell, its arguments and its folder come from the `profiler` table of the manifest, and the object listens to `state`.
     * In other builds, the shell is the `SHELL` variable, with the shell integration when it is available.
     *
     * @param bufferSize                  Device buffer size: the largest number of bytes that one TTY read returns.
     * @param maximumPeriodMilliseconds   Longest wait between two readiness ticks of the TTY, in milliseconds. The call passes one vblank.
     * @param winsize                     Initial window size of the TTY.
     */
    void openTTY (int bufferSize, double maximumPeriodMilliseconds, jam::TerminalWinsize winsize);

    /** @brief Returns the TTY of the platform. */
    static std::unique_ptr<jam::TerminalTTY> createTTY();

    juce::AudioProcessor&
        processor;///< Processor that owns this object. The object reads its wrapper type in profile builds.
    jam::AudioModel& model;///< Model that holds the winsize and the parameters.
    const std::atomic<float>&
        bypass;///< Atomic of the bypass parameter in the model. `process()` runs only while it is 0.
    jam::Model
        state;///< Terminal Model. Its tree is the `terminal` child of the model state. The events store the terminal state into it, and the flush writes it to the tree.
    std::unique_ptr<jam::Model::ParameterAttachment>
        winsizeAttachment;///< Listens to the winsize parameter of `state`. A change calls `restart()`, so the chain prepares again for the new grid only while the clock runs.
    jam::Function::Map<juce::Identifier, void>
        events;///< Event setters that copy the terminal state to the Model lane, keyed by event ID.
    jam::TerminalFIFO input;///< Input lane from the view to the TTY.
    jam::TerminalFIFO commands;///< Command names from the shell to the view.
    jam::VirtualDevice device {
        jam::VirtualDeviceType::virtualDeviceTypeName,
        jam::VirtualDeviceType::virtualDeviceTypeName
    };///< Clock of the chain. This object is its callback.
    std::unique_ptr<jam::TerminalTTY> tty { createTTY() };///< Pseudo terminal of the shell.
    jam::TerminalProcessor
        terminalProcessor;///< Decoder of the TTY output into the rows of `terminalBuffer`.

    jam::TerminalBuffer
        terminalBuffer;///< Row grid that the terminal processor writes. Its ring holds the history rows until `writePendingData()` moves them. Its list of released images feeds the image records.
    jam::TerminalSwap swap;///< Store of the history lines and the live area. The view reads it.

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EVETerminalProcessor)
};
