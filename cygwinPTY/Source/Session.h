#pragma once

#include "Runtime.h"

/**
 * @brief Runs one shell in a pseudo terminal, and relays its bytes and window sizes to the host.
 *
 * The constructor starts a thread. The thread opens the shell, copies the output of the shell to the standard output,
 * and stops the message loop when the output ends. Host input goes to the master of the pseudo terminal.
 */
class Session : private juce::AsyncUpdater
{
public:
    /**
     * @brief Starts the thread that opens the shell and relays its output.
     *
     * @param sessionRuntime  Loaded runtime. The session refers to it, so the runtime must outlive the session.
     * @param sizeHandle      Size pipe from the host. The session takes ownership and closes it.
     * @param packedWinsize   Start size, packed the way jam::TerminalWinsize::pack gives it.
     * @param shellArguments  Shell program, followed by its arguments.
     */
    Session (const Runtime& sessionRuntime, HANDLE sizeHandle, std::int64_t packedWinsize, const juce::StringArray& shellArguments);

    /** @brief Waits for the reader thread to end, then closes the master descriptor. */
    ~Session() override;

private:
    /**
     * @brief Writes the host input to the master, then applies the window sizes.
     *
     * Runs after triggerAsyncUpdate().
     */
    void handleAsyncUpdate() override;

    /**
     * @brief Opens the shell and the relays, then copies the master output to the standard output.
     *
     * The call stops the message loop when it returns. When the shell does not open, the call stops the loop at once.
     * When the relays do not open, the call hangs up the shell, and still copies the output until the output ends.
     */
    void run();

    /**
     * @brief Opens a pseudo terminal with the start size, and starts the shell on its slave.
     *
     * The call makes the slave the controlling terminal of the shell. It ignores the background-terminal signals,
     * puts the shell in its own process group, and makes that group the foreground group. It opens a handle that waits for the shell to end.
     *
     * @return  `true` when the shell starts.
     */
    bool openShell();

    /**
     * @brief Starts the shell with the slave on the standard descriptors, then restores the descriptors.
     *
     * A descriptor that could not be saved is closed, and its Windows standard handle is set again.
     *
     * @param slave  Slave descriptor of the pseudo terminal.
     * @return       Process id of the shell. A value that is not positive means the spawn failed.
     */
    int spawnShell (int slave) const;

    /**
     * @brief Opens the stdio server, the size reader and the shell exit source, and arms the exit source.
     *
     * Host input triggers handleAsyncUpdate(). A stdio end hangs up the shell. Size bytes go to the size ring.
     * The end of the shell calls releaseTerminal().
     *
     * @return  `true` when all three sources open.
     */
    bool openRelays();

    /**
     * @brief Applies each complete packed size in the size ring to the window size of the master.
     *
     * A partial record stays in the ring for the next call.
     */
    void applySizes();

    /** @brief Sends the hangup signal to the shell. */
    void applyHangup();

    /**
     * @brief Runs when the shell exits. Moves the helper into the process group of the shell, then creates a new session.
     */
    void releaseTerminal();

    /**
     * @brief Unpacks a window size from its packed form.
     *
     * @param packed  Packed window size.
     * @return        Window size in the layout of the pseudo terminal.
     */
    static Winsize getWinsize (std::int64_t packed);

    /** @brief Capacity in bytes of the master read buffer, the size ring, and the stdio and size readers. */
    static constexpr int bufferSize { 65536 };

    /** @brief Longest period between two readiness ticks of the stdio and size readers, in milliseconds. */
    static constexpr double maximumPeriodMilliseconds { 1.0 };

    /** @brief Runtime that the session calls. The session does not own it. */
    const Runtime& runtime;
    /** @brief Size pipe from the host. The session closes it. */
    const std::unique_ptr<void, decltype (&CloseHandle)> sizePipe;
    /** @brief Shell program and its arguments, as passed to the constructor. */
    const juce::StringArray arguments;
    /** @brief Window size of the pseudo terminal at its creation. */
    const Winsize startSize;
    /** @brief Master descriptor of the pseudo terminal. openShell() sets it. -1 until then. */
    int master { -1 };
    /** @brief Process id of the shell. openShell() sets it. Not positive until then. */
    int shell { -1 };
    /** @brief Ring of packed window sizes from the size pipe. applySizes() reads it. */
    jam::TerminalFIFO sizes;
    /** @brief Standard streams of the helper. Host input goes to the master, and master output goes to the standard output. */
    jam::StdioServer stdio;
    /** @brief Readiness source of the size pipe. */
    jam::AsyncOutputStream sizeReader;
    /** @brief Read buffer of the master output. run() reads into it. */
    std::array<char, bufferSize> buffer {};
    /** @brief Handle of the reader thread that the constructor starts. The destructor joins it. */
    void* masterReader { nullptr };
    /** @brief Handle of the shell process, opened with SYNCHRONIZE access. */
    std::unique_ptr<void, decltype (&CloseHandle)> shellProcess { nullptr, &CloseHandle };
    /** @brief Readiness source of the shell process. Its callback releases the terminal. */
    jam::AsyncUpdater::Source shellExit;
};
