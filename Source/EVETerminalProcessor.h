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
 * On each `process()` call, the processor writes a changed live area to the swap, unless synchronized output is on.
 * The processor does not decode images. The view reads the bytes of an image from the swap and decodes them.
 * The view pulls through the getter in `userInterfaceGetters`. The getter gives the swap, and the view reads the history lines and the live lines from it.
 * The view sends input bytes through the `terminalInput` event. The bytes wait in `pendingInput`, and they enter the fixed input lane as it has room, in order, on the message thread.
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

    /** @brief Stops the clock through `releaseResources()`. */
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
     * @brief Joins the Model lane to a restored state tree.
     *
     * When @p newState has a `pluginState` child, `state` takes the values of that child, and the child leaves @p newState. A row that the child lacks keeps its value.
     * The tree of `state` then moves from its parent to @p newState.
     *
     * @param newState  State tree that the host restored. It receives the tree of `state`.
     */
    void setState (juce::ValueTree newState);

    /**
     * @brief Copies each property of @p rows to the `terminal` tree of the Model.
     *
     * The call sets the properties on the terminal tree without a restart of the clock.
     *
     * @param rows  Tree whose properties are the rows to write.
     */
    void setRows (const juce::ValueTree& rows);

    /** @brief Takes the message manager lock, then flushes the Model lane to its tree. */
    void flush();

    /**
     * @brief Delivers each complete command name in the command FIFO to a callback.
     *
     * The shell writes the command names to the FIFO. Each name ends with a zero byte.
     * The ring can split a name between two pending blocks.
     * The call joins the parts of a split name.
     * The call skips an empty name.
     * The call runs @p callback once for each complete name.
     *
     * @param callback  Receives the name of each complete command.
     */
    void applyCommands (const std::function<void (const juce::Identifier&)>& callback);

    /**
     * @brief Copies the text of an image record to the system clipboard.
     *
     * The view calls it when the `clipboard` property changes. The call copies nothing when the record holds no bytes.
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
        userInterfaceGetters;///< Getters for the view. The constructor adds the text editor getter and a `previewComponent` getter. The `previewComponent` getter, keyed by `images`, gives the swap only. The text editor getter posts the history writer to the serial context of this object when synchronized output is off, flushes the Model lane, gives the swap, and asks for a process.
    jam::ViewManager::Events
        chainEvents;///< Events of the chain. The constructor adds the `terminalInput` event. It passes its bytes to `addInput()`.

private:
    /**
     * @brief Moves the history rows of the buffer and the image records of the processor into the swap.
     * @return `true` when the call wrote at least one line, or when image records were waiting.
     */
    bool writePendingData();

    /** @brief Starts the clock with this object as the callback, and asks for a process. */
    void start();

    /**
     * @brief Runs on the tick of the clock.
     *
     * It reads `bypass`. When `bypass` is 0, it writes the input lane to the TTY, then the terminal processor reads and decodes the TTY output until a read does not fill its read space.
     * Each call of the terminal processor writes the changed live area to the swap, unless synchronized output is on. The live area has the row count of the current window size.
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

    /** @brief Sizes the input lane, the command FIFO and the decoder with `virtualBlockSize`. When the winsize is valid, it prepares the buffer, the swap and the TTY. */
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
     * @brief Adds the event setters that copy the terminal state to the Model lane, then adds the image events.
     *
     * Each setter calls `storeValue` on `state`. The bool events store 0 or 1. They are `applicationCursor`, `bracketedPaste`,
     * `mouseSgr`, `cursorVisible`, `alternateScreen`, `isRecording`, `shellExited` and `syncOutput`.
     * The `mouseTracking`, `cursorShape`, `cursor` and `numPrompts` events store an int.
     * The `preview`, `title`, `foregroundProcess` and `cwd` events store the text.
     * The `command` event writes the name and a zero byte to `commands`, then flushes `commands`. It then increments `numCommands` in `state`.
     * It does this only when the name is not empty and `commands` has room for it.
     */
    void registerEvents();

    /**
     * @brief Adds the image events: `hyperlink`, `clipboard`, `previewBox` and `outputBlock`.
     *
     * The `hyperlink` event does nothing.
     * The `clipboard` event stores the image number in `pendingClipboard`. `handleAsyncUpdate()` then stores it in the Model lane.
     * The `previewBox` event stores its value in the `previewBox` property of `state`.
     */
    void registerImageEvents();

    /**
     * @brief Adds the text editor getter keyed by the winsize type, and the `previewComponent` getter keyed by the images type.
     *
     * The text editor getter posts the history writer to the serial context of this object when synchronized output is off, flushes the Model lane, gives the swap, and asks for a process.
     * The `previewComponent` getter gives the swap only.
     */
    void registerUserInterfaceGetters();

    /** @brief Adds the `terminalInput` event to the chain events. The event passes its bytes and its size to `addInput()`. */
    void registerChainEvents();

    /**
     * @brief Adds bytes to `pendingInput`, then runs applyPendingInput(). Input of any size arrives whole and in order. Message thread.
     *
     * @param bytes  Bytes to write.
     * @param size   Number of bytes.
     */
    void addInput (const char* bytes, int size);

    /**
     * @brief Moves the bytes of `pendingInput` that fit into the input lane, then asks the device for a process.
     *
     * The lane is the fixed lock-free handoff to the tick. `addInput()` and the text editor getter, on each vblank, run this call. Message thread.
     */
    void applyPendingInput();

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
     * In a profile standalone build, the shell, its arguments and its folder come from the `profiler` table of the manifest.
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

    /** @brief Returns the TTY of the platform. */
    static std::unique_ptr<jam::TerminalTTY> createTTY();

    juce::AudioProcessor&
        processor;///< Processor that owns this object. The object reads its wrapper type in profile builds.
    jam::AudioModel& model;///< Model that holds the winsize and the parameters.
    const std::atomic<float>&
        bypass;///< Atomic of the bypass parameter in the model. `process()` runs only while it is 0.
    jam::Model
        state;///< Terminal Model. Its tree is the `terminal` child of the model state. The events store the terminal state into it, and the flush writes it to the tree.
    const juce::Identifier terminalTag { jam::Format::toType (Id::terminal) };///< Identifier of the `terminal` tree. It is built at construction, off the clock, so that the tick of the clock does not build it.
    std::unique_ptr<jam::Model::ParameterAttachment>
        winsizeAttachment;///< Listens to the winsize parameter of `state`. A change calls `restart()`, so the chain prepares again for the new grid only while the clock runs.
    jam::Function::Map<juce::Identifier, void>
        events;///< Event setters that copy the terminal state to the Model lane, keyed by event ID.
    std::string pendingInput;    ///< Input bytes that wait for room in the input lane. Message thread only.
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
