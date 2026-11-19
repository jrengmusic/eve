#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"

/**
 * @class EVETerminalProcessor
 * @brief Terminal chain of the plugin. It decodes the TTY output into the rows of a `jam::TerminalBuffer` and publishes them through a `jam::TerminalSwap`.
 *
 * `process()` runs the `jam::TerminalProcessor`. The processor decodes the TTY output into the rows of the buffer.
 * `writePendingData()` moves the history rows of the buffer and the image records of the processor into the swap.
 * The view pulls through the getter in `userInterfaceGetters`. The getter gives the swap, the live slots lane and the Model.
 * The view sends input bytes through the `write` event under the `terminalInput` key of the chain events. The bytes wait in `pendingInput`, and they enter the fixed input lane as it has room, in order.
 *
 * @par Model lane
 * The object owns a terminal `jam::Model`, `state`. Its tree is the `terminal` child of the APVTS state of the model.
 * The constructor adds the parameters of the `parametersLayout` document to `state` through `jam::ModelLayout::addParameters`.
 * The events store each value on `state`. The decoder fires them on the clock. The swap write in `handleAsyncUpdate()` fires the grid events and the boundary events of the swap.
 * The Model does not run its own timer. The text editor getter in `userInterfaceGetters` flushes it, `prepare()` flushes it at its end, and `flush()` flushes it under the message manager lock.
 *
 * @par Clock
 * The object owns its clock, `jam::VirtualDevice device`. The object is the `juce::AudioIODeviceCallback` of the clock.
 * The tick, `audioDeviceIOCallbackWithContext`, runs `process()` on the serial context of the device.
 * The TTY readiness source runs on the same context. Its callback calls `device.clock (numSamples)`.
 * `numSamples` is the number of bytes that the TTY reports as ready.
 * The clock opens and starts only after a TTY exists. `prepare()` opens the TTY for a valid window size, and `process()` runs only on that clock.
 *
 * @par Thread contract
 * `process()` runs on the serial context of the device. It writes the input lane to the TTY, then decodes the TTY output.
 * A winsize change calls `prepare()` from the processor. `prepare()` starts the clock only for a valid window size and an open TTY.
 * The lanes between the threads belong to the object: `input`, `commands`, `gridEvents`, `images` and `liveSlots`. Each worker receives its lane as an argument.
 */
class EVETerminalProcessor
    : private jam::AsyncUpdater
    , private juce::AudioIODeviceCallback
{
public:
    /**
     * @brief Builds the Model lane, registers the events, the user interface getters and the chain events.
     *
     * The constructor binds `bypass` to the atomic of the bypass parameter in the model.
     * It builds `state` on the `terminal` child of the model state, stops the timer of `state`, and adds the parameters of the `parametersLayout` document to it.
     *
     * @param processorToUse  Processor that owns this object. The object reads its wrapper type in profile builds.
     * @param modelToUse      Model that holds the bypass parameter and the history capacity.
     */
    EVETerminalProcessor (juce::AudioProcessor& processorToUse, jam::AudioModel& modelToUse);

    /** @brief Destroys the members. The destructor does not call `releaseResources()`, which `EVEProcessor::releaseResources()` calls. */
    ~EVETerminalProcessor() override;

    /**
     * @brief Stops the clock, then opens the TTY and the clock for a valid window size.
     *
     * The sample rate is 48000 Hz.
     * A valid window size opens the TTY when `tty` is empty, with one vblank as its longest readiness period.
     * When the call opens the TTY, it then opens the window size work on the device context.
     * The clock opens and starts only when `tty` holds a TTY. It takes the buffer size of that TTY, from `getDefaultBufferSize()`.
     * Without a valid window size, the clock stays stopped.
     */
    void prepare();

    /**
     * @brief Stops the clock, cancels the pending update, then closes the clock, the window size work and the TTY.
     *
     * The pending update writes the swap, and it can request a process. It does not run after the clock closes.
     */
    void releaseResources();

    /**
     * @brief Clears the terminal buffer with the clock stopped.
     *
     * The call runs the pending update before it clears the buffer. It ends with prepare(), which starts the clock again when the window size is valid and a TTY exists.
     */
    void clearHistory();

    /**
     * @brief Joins the Model lane to a restored state tree.
     *
     * When @p newState has a `terminal` child, `state` takes the values of that child, and the child leaves @p newState.
     * The tree of `state` then moves from its parent to @p newState.
     *
     * @param newState  State tree that the host restored. It receives the tree of `state`.
     */
    void setState (juce::ValueTree newState);

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
     * The view runs the call when the numGridEvents or the firstLine property changes. The tree of state changes only on the message thread, so the call runs there.
     * The call then drops the grid starts that lie before the `firstLine` row of `state`. When the active grid start lies before that row, it takes the value of the first remaining grid start.
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
     * The line is the command, one space, the path quoted for the shell in the `shell` row of `state`, and a carriage return.
     * The call sends the line through addInput().
     *
     * @param command  Command text of the open command.
     * @param path     Decoded file path to open.
     */
    void applyOpenCommand (std::string_view command, std::string_view path);

    /**
     * @brief Returns the paths quoted for the shell in the `shell` row of `state`, joined by one space.
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
        userInterfaceGetters;///< Getters for the view. The constructor adds the text editor getter and a `previewComponent` getter. The `previewComponent` getter, keyed by `images`, gives the swap and the Model. The text editor getter, keyed by `winsize`, flushes the Model lane, triggers the async update when synchronized output is off, gives the swap, the live slots and the Model, and runs applyPendingInput().
    jam::ViewManager::Events
        chainEvents;///< Events of the chain. The outer key is the event name, and the inner key is the event ID. The constructor adds the `write` event under `terminalInput`. It passes its bytes to `addInput()`.

private:
    /**
     * @brief Moves the history rows of the buffer and the image records of the processor into the swap.
     *
     * The call passes the raw atomics of the `heldLine` and `heldImage` parameters of `state` to the swap, which loads them.
     * @return  `true` when the swap wrote at least one line, or when image records were waiting.
     */
    bool writePendingData();

    /** @brief Starts the clock with this object as the callback, and asks for a process. */
    void start();

    /**
     * @brief Runs on the tick of the clock.
     *
     * It reads `bypass`. When `bypass` is 0, it writes the input lane to the TTY, then the terminal processor decodes the TTY output
     * for as long as its call returns `true`. The call needs no test for an open TTY: `start()` runs only after `prepare()` has opened a TTY, and the clock calls back only while it plays.
     *
     * It reads the `background` parameter of the `terminal` tree through `terminalTag`, and it passes the value to the terminal processor, with the lanes `images` and `liveSlots`.
     */
    void process() noexcept;

    /** @brief The tick of the clock. It runs `process()`. */
    void audioDeviceIOCallbackWithContext (const float* const*,
                                           int,
                                           float* const*,
                                           int,
                                           int,
                                           const juce::AudioIODeviceCallbackContext&) override;

    /**
     * @brief Sizes the input lane, the command FIFO, the gridEvents lane and the decoder with `virtualBlockSize`.
     *
     * When the winsize is valid, it runs prepare(winsize, historyCapacity), which prepares the buffer and the swap, and submits the TTY window size.
     * It does not open the TTY.
     */
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
     * @brief Adds the clipboard event and the events that store 64-bit row values of the `terminal` tree of `state`.
     *
     * The `clipboard` event stores the image number in `pendingClipboard`. `handleAsyncUpdate()` then stores it in the Model lane.
     * Each event of the list below stores its value through the `jam::Parameter<int64_t>` of its row, with `store()`. The events are
     * `previewBox`, `outputBlock`, `scrollingRegion`, `pen`, `savedPenNormal`, `savedPenAlternate`, `keyboardModesNormal`,
     * `keyboardModesAlternate`, `syncStart`, `firstLine`, `endLine`, `closedLine`, `lineIndex`, `firstImage`, `endImage` and `imageIndex`.
     * Each id must name an int64 row of the `terminal` tree (jassert).
     */
    void registerImageEvents();

    /**
     * @brief Adds the setters of the grid events: gridStart and removedGridStart.
     *
     * The setters run inside the swap write of writePendingData(), which handleAsyncUpdate() calls.
     * Each setter writes its index in gridEventTypes and its line to gridEvents when the lane has room for both parts.
     * It then flushes the lane and raises numGridEvents with storeValue.
     *
     * A Model row keeps only the last value stored before a flush. The lane carries every event of a write instead.
     */
    void registerGridEvents();

    /**
     * @brief Adds the text editor getter keyed by the winsize type, and the `previewComponent` getter keyed by the images type.
     *
     * The text editor getter flushes the Model lane, triggers the async update when synchronized output is off, gives the swap, the live slots lane and the Model, and runs applyPendingInput().
     * The `previewComponent` getter gives the swap and the Model.
     */
    void registerUserInterfaceGetters();

    /** @brief Adds the `write` event under the `terminalInput` key of the chain events. The event passes its bytes and its size to `addInput()`. */
    void registerChainEvents();

    /**
     * @brief Writes the bytes that fit the input lane, appends the rest to `pendingInput`, then runs applyPendingInput().
     *
     * The bytes enter the lane directly only when `pendingInput` is empty, so that the order of the bytes is kept.
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
     * @brief Prepares the buffer, the swap and the decoder for a window size, and submits the TTY window size.
     *
     * The call runs handleUpdateNow() three times. The first run precedes applyWinsize(), the second run follows applyWinsize(), and the third run follows the call to swap.setCapacity().
     * Each run queues the async update and waits for it. The update writes the pending history rows to the swap, and the swap write reports the boundaries of the swap.
     * Between the first and the second run, the decoder applies the window size to the buffer through applyWinsize().
     * On the normal screen, applyWinsize() scrolls the buffer up by the rows that the cursor exceeds the last row of the new winsize, and it moves the cursor up by the same count.
     * The call then sizes the buffer for the blocks of one vblank, prepares the swap with the byte count of the buffer, resets the decoder, and sets the history capacity.
     * It then submits the TTY window size with `ttyResize.submit()`, and calls flush(), which flushes the Model lane under the message manager lock.
     *
     * @param winsize          Window size of the grid.
     * @param historyCapacity  Number of history lines that the swap keeps.
     */
    void prepare (jam::TerminalWinsize winsize, int historyCapacity);

    /**
     * @brief Opens the TTY on the serial context of the device.
     *
     * In a profile standalone build, the shell and its arguments come from the `profiler` table of the manifest. The folder is the folder of the manifest file. The call stores the file URL of that folder in the `cwd` row of `state`, and the program in the `shell` row. The call opens the TTY without a check. A failed open stores 1 in the `shellExited` property of `state`, and leaves `tty` empty.
     * In other builds, the call creates a new TTY and opens it. `prepare()` calls it only when `tty` is empty. It takes the program from the first token of getShell(), without its quotes. The other tokens are the arguments.
     * In other builds, the call writes the integration scripts with ShellIntegration::addShellScripts(). Each open appends ShellIntegration::getShellArguments() to the arguments, and it passes ShellIntegration::getShellEnvironment() as the environment.
     * In other builds, the shell starts in the directory of the `cwd` row of `state` when that row holds a file URL. Otherwise it starts in ShellIntegration::getStartDirectory(). The call then stores the file URL of the start directory in the `cwd` row.
     * In other builds, on Windows, when the program does not open, the call tries `powershell.exe` and then `cmd.exe`, without the configured arguments. The shell integration arguments still follow. The first one that opens runs. On the other platforms, no fallback runs.
     * In other builds, the `shell` row of `state` holds the program that runs. When no program opens, the row holds the configured program.
     * In other builds, the `shellNotFound` row holds the configured program when a fallback runs. The row is empty otherwise.
     * In other builds, when no program opens, `shellExited` is set to 1, and `tty` is left empty.
     *
     * The call does not size the clock. `prepare()` takes the buffer size of the TTY from `getDefaultBufferSize()`.
     *
     * @param maximumPeriodMilliseconds   Longest wait between two readiness ticks of the TTY, in milliseconds. The call passes one vblank.
     * @param winsize                     Initial window size of the TTY.
     */
    void openTTY (double maximumPeriodMilliseconds, jam::TerminalWinsize winsize);

    /**
     * @brief Returns the shell command line that the TTY starts.
     *
     * In a profile build, the command is the `shell` property of the `profiler` table of the manifest. When the property is absent, the default shell applies.
     * In other builds, the command is the `shell` setting, which may hold arguments after the program. When the setting is absent or empty, the default shell applies.
     * The default shell is `C:/msys64/usr/bin/zsh.exe -l` on Windows. On the other platforms, it is the `SHELL` variable, or `/bin/zsh` when that variable is not set.
     *
     * @return The shell command line. openTTY() splits it into the program and its arguments.
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
     * @brief Returns the pseudo terminal of the platform, for the program that the shell starts.
     *
     * On Windows, the call gives a Cygwin TTY when getRuntime() finds the runtime next to @p program.
     * Otherwise the call gives a Windows console TTY, whose `setInputMode` and `setOutputMode` callbacks store the console modes in `state`. On other platforms, it gives a Unix TTY.
     * When EVE_USING_REPLAY is set, the platform TTY is wrapped in a TerminalMemoryTTY.
     * The readiness callback of the returned TTY clocks the device.
     *
     * @param program    Shell program. On Windows, the runtime is found only for an absolute path. An empty program gives the Windows console TTY.
     * @param directory  Folder that receives the unpacked helper files. Only the Cygwin TTY uses it.
     * @return           The TTY, with its readiness callback set.
     */
    std::unique_ptr<jam::TerminalTTY> createTTY (const juce::String& program, const juce::File& directory);

#if JUCE_WINDOWS
    /**
     * @brief Returns the runtime DLL that sits next to the program, or an empty File.
     *
     * The candidates are the msys runtime and the cygwin runtime from the settings, with the shipped defaults.
     * The first candidate whose file name sits in the folder of @p program is returned.
     *
     * @param program  Shell program, as an absolute path. A relative path gives an empty File.
     * @return         Runtime file, or an empty File when no candidate is found.
     */
    static juce::File getRuntime (const juce::String& program);
#endif

    juce::AudioProcessor&
        processor;///< Processor that owns this object. The object reads its wrapper type in profile builds.
    jam::AudioModel& model;///< Model that holds the parameters, including the bypass parameter and the history capacity.
    const std::atomic<float>&
        bypass;///< Atomic of the bypass parameter in the model. `process()` runs only while it is 0.
    jam::Model
        state;///< Terminal Model. Its tree is the `terminal` child of the model state. The events store the terminal state into it, and the flush writes it to the tree.
    const juce::Identifier terminalTag { jam::Format::toType (Id::terminal) };///< Identifier of the `terminal` tree, built at construction.
    jam::TerminalProcessor::Events
        events;///< Event setters that copy the terminal state to the Model lane, keyed by event ID.
    std::string pendingInput;    ///< Input bytes that wait for room in the input lane.
    size_t pendingInputOffset { 0 };    ///< Index of the first byte of `pendingInput` that waits for room in the input lane. The bytes before it already entered the lane.
    jam::TerminalFIFO input;///< Input lane from the view to the TTY.
    jam::TerminalFIFO commands;///< Command names from the shell to the view.
    jam::TerminalFIFO gridEvents;///< @brief Carries the grid events from handleAsyncUpdate() to applyGridEvents().
    jam::TerminalFIFO images;///< Lane of the image records from the decoder to the swap. The decoder writes it, and writePendingData() consumes it through the swap.
    juce::AbstractFifo liveSlots { jam::TerminalSwap::numLiveSlots };///< Lane of the live slots. The decoder fills a slot through the swap, and the text editor getter hands the lane to the reader, which takes a ready slot.
    const std::array<juce::Identifier, 2> gridEventTypes { Id::gridStart, Id::removedGridStart };    ///< The two identifiers select the list update of each index. The position of each identifier is the index that its event writes to `gridEvents`. Index 0 appends a grid start. Index 1 removes the grid start at a line.
    jam::TerminalProcessor
        terminalProcessor;///< Decoder of the TTY output into the rows of `terminalBuffer`.

    jam::TerminalBuffer
        terminalBuffer;///< Row grid that the terminal processor writes. `writePendingData()` moves its history rows to the swap.
    jam::TerminalSwap swap;///< Store of the history lines and the live area. The view reads it.
    jam::VirtualDevice device {
        jam::VirtualDeviceType::virtualDeviceTypeName,
        jam::VirtualDeviceType::virtualDeviceTypeName
    };///< Clock of the chain. This object is its callback.
    std::unique_ptr<jam::TerminalTTY> tty;///< Pseudo terminal of the shell. It is empty until openTTY() opens a session, and it is empty again when the open fails. releaseResources() resets it, and each TTY destructor closes its session.
    jam::AsyncUpdater::Work ttyResize;///< Work of the session that sets the TTY window size on the device context. prepare() opens it on the device context when the TTY opens, and submits it for each window size. Its callback reads the winsize row of state.

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EVETerminalProcessor)
};
