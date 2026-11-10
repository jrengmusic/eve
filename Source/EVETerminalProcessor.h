#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"

/**
 * @class EVETerminalProcessor
 * @brief Terminal chain of the plugin. It decodes the TTY output into the rows of a `jam::TerminalBuffer` and publishes them through a `jam::TerminalSwap`.
 *
 * `process()` runs the `jam::TerminalProcessor`. The processor decodes the TTY output into the rows of the buffer.
 * `writePendingData()` moves the history rows of the buffer and the image records of the processor into the swap.
 * The view pulls through the getter in `userInterfaceGetters`. The getter gives the swap.
 * The view sends input bytes through the `write` event under the `terminalInput` key of the chain events. The bytes wait in `pendingInput`, and they enter the fixed input lane as it has room, in order.
 *
 * @par Model lane
 * The object owns a terminal `jam::Model`, `state`. Its tree is the `terminal` child of the APVTS state of the model.
 * The constructor adds the parameters of the `parametersLayout` document to `state` through `jam::ModelLayout::addParameters`.
 * The events call `storeValue` on `state`. The decoder fires them on the clock. The swap write in `handleAsyncUpdate()` fires the grid events.
 * The Model does not run its own timer. The text editor getter in `userInterfaceGetters` flushes it, and `flush()` flushes it under the message manager lock.
 *
 * @par Clock
 * The object owns its clock, `jam::VirtualDevice device`. The object is the `juce::AudioIODeviceCallback` of the clock.
 * The tick, `audioDeviceIOCallbackWithContext`, runs `process()` on the serial context of the device.
 * The TTY readiness source runs on the same context. Its callback calls `device.clock (numSamples)`.
 * `numSamples` is the number of bytes that the TTY reports as ready.
 *
 * @par Thread contract
 * `process()` runs on the serial context of the device. It writes the input lane to the TTY, then decodes the TTY output.
 * The winsize attachment of `state` calls `restart()` when the winsize changes. A stopped clock stays stopped, so a winsize change prepares the chain only while the clock runs.
 */
class EVETerminalProcessor
    : private jam::AsyncUpdater
    , private juce::AudioIODeviceCallback
{
public:
    /**
     * @brief Builds the Model lane, registers the events, the user interface getters and the chain events, then sets the TTY readiness to clock the device.
     *
     * The constructor binds `bypass` to the atomic of the bypass parameter in the model.
     * It builds `state` on the `terminal` child of the model state, stops the timer of `state`, and adds the parameters of the `parametersLayout` document to it.
     *
     * @param processorToUse  Processor that owns this object. The object reads its wrapper type in profile builds.
     * @param modelToUse      Model that holds the bypass parameter and the history capacity.
     */
    EVETerminalProcessor (juce::AudioProcessor& processorToUse, jam::AudioModel& modelToUse);

    /** @brief Stops the clock through `releaseResources()`. */
    ~EVETerminalProcessor() override;

    /**
     * @brief Stops the clock, opens it at the fixed sample rate and the device buffer size of the OS, then starts it unconditionally.
     *
     * The sample rate is 48000 Hz. The device buffer size is `virtualBlockSize` on Windows and 1024 on the other systems.
     */
    void prepare();

    /** @brief Stops the clock, then closes the device. */
    void releaseResources();

    /**
     * @brief Stops the clock, calls @p callback, then starts the clock again only when it ran before the call.
     *
     * A stopped clock stays stopped.
     *
     * @tparam Callback  Callable type with no argument.
     * @param callback   Callable that runs while the clock is stopped.
     */
    template <typename Callback>
    void restart (Callback&& callback)
    {
        const auto isPlaying { device.isPlaying() };

        device.stop();
        callback();

        if (isPlaying)
            start();
    }

    /** @brief Restarts with no change. The chain prepares again from the winsize of `state` and the history capacity of the model, when the clock starts. */
    void restart();

    /** @brief Clears the history rows of the terminal buffer inside a restart, while the clock is stopped. */
    void clearHistory();

    /**
     * @brief Joins the Model lane to a restored state tree.
     *
     * When @p newState has a `pluginState` child, `state` takes the values of that child, and the child leaves @p newState.
     * The tree of `state` then moves from its parent to @p newState.
     *
     * @param newState  State tree that the host restored. It receives the tree of `state`.
     */
    void setState (juce::ValueTree newState);

    /**
     * @brief Copies each property of @p rows to the `terminal` tree of the Model.
     *
     * The call sets the properties on the terminal tree and does not restart the clock.
     *
     * @param rows  Tree whose properties are the rows to write.
     */
    void setRows (const juce::ValueTree& rows);

    /** @brief Takes the message manager lock, then flushes the Model lane to its tree. */
    void flush();

    /**
     * @brief Delivers each complete command name in the command FIFO to a callback.
     *
     * The command event writes the command names to the FIFO. Each name ends with a zero byte.
     * The ring can split a name between two pending blocks.
     * The call joins the parts of a split name.
     * The call skips an empty name.
     * The call runs @p callback once for each complete name.
     *
     * @param callback  Receives the name of each complete command.
     */
    void applyCommands (const std::function<void (const juce::Identifier&)>& callback);

    /**
     * @brief Applies the grid events of the gridEvents lane to the gridStarts child of state.
     *
     * One write of the swap can queue several grid events, so the call applies each complete event in the lane.
     * The view runs the call when the numGridEvents property changes. The tree of state changes only on the message thread, so the call runs there.
     */
    void applyGridEvents();

    /**
     * @brief Copies the text of an image record to the system clipboard.
     *
     * The view calls it when the `clipboard` property changes.
     *
     * @param image  Number of the image record that holds the text.
     */
    void copyToClipboard (juce::int64 image);

    /**
     * @brief Types an open command with a file path into the shell, then submits the line.
     *
     * The line is the command, one space, the path quoted for the shell of this processor, and a carriage return.
     * The call sends the line through addInput().
     *
     * @param command  Command text of the open command.
     * @param path     Decoded file path to open.
     */
    void applyOpenCommand (std::string_view command, std::string_view path);

    /**
     * @brief Returns the paths quoted for the shell of this processor, joined by one space.
     * @param paths  The absolute paths.
     * @return       UTF-8 text, for the paste of a file drop.
     */
    std::string getQuotedPaths (const juce::StringArray& paths) const;

#if JAM_USING_PROFILER
    /**
     * @brief Returns the manifest file of a profile build.
     * @return File that the first command-line argument names, resolved against the working folder.
     */
    static juce::File getManifestFile();
#endif

    jam::ViewManager::Getters
        userInterfaceGetters;///< Getters for the view. The constructor adds the text editor getter and a `previewComponent` getter. The `previewComponent` getter, keyed by `images`, gives the swap only. The text editor getter, keyed by `winsize`, flushes the Model lane, triggers the async update when synchronized output is off, gives the swap, and runs applyPendingInput().
    jam::ViewManager::Events
        chainEvents;///< Events of the chain. The outer key is the event name, and the inner key is the event ID. The constructor adds the `write` event under `terminalInput`. It passes its bytes to `addInput()`.

private:
    /**
     * @brief Moves the history rows of the buffer and the image records of the processor into the swap.
     * @return `true` when the swap wrote at least one line, or when image records were waiting.
     */
    bool writePendingData();

    /** @brief Starts the clock with this object as the callback, and asks for a process. */
    void start();

    /**
     * @brief Runs on the tick of the clock.
     *
     * It reads `bypass`. When `bypass` is 0, it writes the input lane to the TTY, then the terminal processor decodes the TTY output
     * for as long as its call returns `true`.
     *
     * It reads the `background` parameter of the `terminal` tree through `terminalTag`, and it passes the value to the terminal processor.
     */
    void process() noexcept;

    /** @brief The tick of the clock. It runs `process()`. */
    void audioDeviceIOCallbackWithContext (const float* const*,
                                           int,
                                           float* const*,
                                           int,
                                           int,
                                           const juce::AudioIODeviceCallbackContext&) override;

    /** @brief Sizes the input lane, the command FIFO, the gridEvents lane and the decoder with `virtualBlockSize`. When the winsize is valid, it prepares the buffer, the swap and the TTY. */
    void audioDeviceAboutToStart (juce::AudioIODevice*) override;

    /** @brief Does nothing. */
    void audioDeviceStopped() override;

    /**
     * @brief Writes the pending history rows to the swap, then stores the pending clipboard image number in the `clipboard` property of `state`.
     *
     * The call reads the `pendingClipboard` parameter of the `terminal` tree before it runs `writePendingData()`.
     * It asks the device for a process when `writePendingData()` returns `true`.
     * The `clipboard` property is stored on each call, whether or not an image waits.
     */
    void handleAsyncUpdate() override;

    /**
     * @brief Adds the event setters that copy the terminal state to the Model lane.
     *
     * Each event of the list below takes an int. `storeValue` on `state` stores it under the key of the event. The events are
     * `applicationCursor`, `bracketedPaste`, `mouseSgr`, `cursorVisible`, `alternateScreen`, `isRecording`, `shellExited`, `syncOutput`,
     * `autoWrap`, `originMode`, `insertMode`, `isWrapPending`, `mouseTracking`, `cursorShape`, `keyboardFlags`, `cursor`, `numPrompts`,
     * `activeLink`, `keyboardFlagsInactive`, `numKeyboardModesNormal`, `numKeyboardModesAlternate`, `savedCursorNormal` and `savedCursorAlternate`.
     * The `preview`, `title`, `foregroundProcess` and `cwd` events store the text.
     * The `command` event writes the name and a zero byte to `commands`, then flushes `commands`. It then increments `numCommands` in `state`.
     * It does this only when the name is not empty and `commands` has room for it.
     * The call then runs registerImageEvents() and registerGridEvents().
     */
    void registerEvents();

    /**
     * @brief Adds the clipboard event and the events that store 64-bit values on the `terminal` tree of `state`.
     *
     * The `clipboard` event stores the image number in `pendingClipboard`. `handleAsyncUpdate()` then stores it in the Model lane.
     * The `previewBox`, `outputBlock`, `scrollingRegion`, `pen`, `savedPenNormal`, `savedPenAlternate`, `keyboardModesNormal`,
     * `keyboardModesAlternate` and `syncStart` events store their value on the `terminal` tree of `state`.
     */
    void registerImageEvents();

    /**
     * @brief Adds the setters of the grid events: gridStart, removedGridStart and firstLine.
     *
     * The setters run on the serial context of handleAsyncUpdate().
     * Each setter writes its name and its line to gridEvents when the lane has room for both parts.
     * It then flushes the lane and raises numGridEvents with storeValue.
     *
     * A Model row keeps only the last value stored before a flush. The lane carries every event of a write instead.
     */
    void registerGridEvents();

    /**
     * @brief Adds the text editor getter keyed by the winsize type, and the `previewComponent` getter keyed by the images type.
     *
     * The text editor getter flushes the Model lane, triggers the async update when synchronized output is off, gives the swap, and runs applyPendingInput().
     * The `previewComponent` getter gives the swap only.
     */
    void registerUserInterfaceGetters();

    /** @brief Adds the `write` event under the `terminalInput` key of the chain events. The event passes its bytes and its size to `addInput()`. */
    void registerChainEvents();

    /**
     * @brief Adds bytes to `pendingInput`, then runs applyPendingInput().
     *
     * @param bytes  Bytes to write.
     * @param size   Number of bytes.
     */
    void addInput (const char* bytes, int size);

    /**
     * @brief Moves the bytes of `pendingInput` that fit into the input lane, then asks the device for a process.
     *
     * `addInput()` and the text editor getter run this call.
     */
    void applyPendingInput();

    /** @brief Returns the history capacity that the `historyCapacity` component of the model holds. */
    int getHistoryCapacity() const;

    /** @brief Returns the winsize of the `terminal` tree of `state`, unpacked. */
    jam::TerminalWinsize getWinsize() const;

    /**
     * @brief Prepares the buffer, the swap, the decoder and the TTY for a window size.
     *
     * The call runs handleUpdateNow() three times. The first run precedes applyWinsize(), the second run follows applyWinsize(), and the third run follows the call to swap.setCapacity().
     * Each run queues the async update and waits for it. The update writes the pending history rows to the swap, and the swap write emits the firstLine event.
     * Between the first and the second run, the decoder applies the window size to the buffer through applyWinsize().
     * On the normal screen, applyWinsize() scrolls the buffer up by the rows that the cursor exceeds the last row of the new winsize, and it moves the cursor up by the same count.
     * The call then sizes the buffer for the blocks of one vblank, prepares the swap with the byte count of the buffer, resets the decoder, and sets the history capacity.
     * It then opens the TTY and sets its window size.
     *
     * @param winsize          Window size of the grid.
     * @param historyCapacity  Number of history lines that the swap keeps.
     */
    void prepare (jam::TerminalWinsize winsize, int historyCapacity);

    /**
     * @brief Opens the TTY on the serial context of the device.
     *
     * In a profile standalone build, the shell, its arguments and its folder come from the `profiler` table of the manifest, and the TTY is opened without the check below.
     * In other builds, the shell is the `SHELL` variable, with the shell integration when it is available. The call opens the TTY only when it is not open.
     * In other builds, the shell starts in the directory of the `cwd` row of `state`. An empty `cwd` row keeps `ShellIntegration::getStartDirectory()`.
     *
     * A failed open stores 1 in the `shellExited` property of `state`.
     *
     * @param bufferSize                  Device buffer size: the largest number of bytes that one TTY read returns.
     * @param maximumPeriodMilliseconds   Longest wait between two readiness ticks of the TTY, in milliseconds. The call passes one vblank.
     * @param winsize                     Initial window size of the TTY.
     */
    void openTTY (int bufferSize, double maximumPeriodMilliseconds, jam::TerminalWinsize winsize);

    /**
     * @brief Returns the shell that the TTY starts.
     *
     * In a profile build, the shell is the `shell` key of the `profiler` table of the manifest.
     * In other builds, the shell is the `SHELL` variable.
     * The fallback is the `SHELL` variable, or the default shell of the platform when that variable is not set:
     * `powershell.exe` on Windows, and `/bin/zsh` on the other platforms.
     *
     * @return The shell program.
     */
    juce::String getShell() const;

#if JAM_USING_PROFILER
    /**
     * @brief Returns the `profiler` child of the `manifest` tree in the manifest file.
     *
     * In a standalone build, the manifest file is the command-line file. In another build, the processor fills the manifest file through the `jam::ENAXClientExtensions`, under `Id::manifestFile`.
     *
     * @return The `profiler` tree of the manifest file.
     */
    juce::ValueTree getProfilerTable() const;
#endif

    /**
     * @brief Returns the TTY of the platform.
     *
     * When EVE_USING_REPLAY is set, the platform TTY is wrapped in a TerminalMemoryTTY.
     */
    static std::unique_ptr<jam::TerminalTTY> createTTY();

    juce::AudioProcessor&
        processor;///< Processor that owns this object. The object reads its wrapper type in profile builds.
    jam::AudioModel& model;///< Model that holds the parameters, including the bypass parameter and the history capacity.
    const std::atomic<float>&
        bypass;///< Atomic of the bypass parameter in the model. `process()` runs only while it is 0.
    jam::Model
        state;///< Terminal Model. Its tree is the `terminal` child of the model state. The events store the terminal state into it, and the flush writes it to the tree.
    const juce::Identifier terminalTag { jam::Format::toType (Id::terminal) };///< Identifier of the `terminal` tree, built at construction.
    std::unique_ptr<jam::Model::ParameterAttachment>
        winsizeAttachment;///< Listens to the winsize parameter of `state`. A change calls `restart()`, so the chain prepares again for the new grid only while the clock runs.
    jam::TerminalProcessor::Events
        events;///< Event setters that copy the terminal state to the Model lane, keyed by event ID.
    std::string pendingInput;    ///< Input bytes that wait for room in the input lane.
    jam::TerminalFIFO input;///< Input lane from the view to the TTY.
    jam::TerminalFIFO commands;///< Command names from the shell to the view.
    jam::TerminalFIFO gridEvents;///< @brief Carries the grid events from handleAsyncUpdate() to applyGridEvents().
    jam::VirtualDevice device {
        jam::VirtualDeviceType::virtualDeviceTypeName,
        jam::VirtualDeviceType::virtualDeviceTypeName
    };///< Clock of the chain. This object is its callback.
    std::unique_ptr<jam::TerminalTTY> tty { createTTY() };///< Pseudo terminal of the shell.
    jam::TerminalProcessor
        terminalProcessor;///< Decoder of the TTY output into the rows of `terminalBuffer`.

    jam::TerminalBuffer
        terminalBuffer;///< Row grid that the terminal processor writes. `writePendingData()` moves its history rows to the swap.
    jam::TerminalSwap swap;///< Store of the history lines and the live area. The view reads it.

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EVETerminalProcessor)
};
