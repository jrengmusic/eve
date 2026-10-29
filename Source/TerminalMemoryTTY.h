#pragma once
#include <JuceHeader.h>

/**
 * @brief Replay TTY. It records the shell output through the platform TTY, then serves the record from memory.
 *
 * `open()` starts the shell on the platform TTY and waits until the shell closes its output.
 * The TTY appends each block that the platform TTY delivers to the record.
 * The `onBytesReady` of the platform TTY ignores its `numSamples` and starts its own update.
 * After the record is complete, `read()` serves it from memory in blocks. `read()` returns -1 at the end.
 * The TTY ignores input and window size changes.
 */
class TerminalMemoryTTY : public jam::TerminalTTY,
                          private jam::AsyncUpdater
{
public:
    /**
     * @brief Takes the platform TTY that records the shell output.
     *
     * @param platformTTY  TTY that runs the shell. The object owns it.
     */
    explicit TerminalMemoryTTY (std::unique_ptr<jam::TerminalTTY> platformTTY);

    /**
     * @brief Opens the platform TTY, records its output, then starts the replay.
     *
     * The call blocks until the platform TTY reports the end of the shell output, for at most `recordTimeoutMs`.
     * It then makes the stream of the record. With the profiler enabled, it drains and clears the profiler,
     * so the profile holds the replay only. On macOS it then calls `onBytesReady (0)` on @p context.
     * `onBytesReady` must be set (jassert). When the stream exists, the call changes nothing and returns `true`.
     *
     * @param context           Serial context for the readiness source of the platform TTY and for the first `onBytesReady` of the replay (macOS).
     * @param bufferSize        Device buffer size. It sizes the block that records the platform output. The call passes it to the platform TTY.
     * @param maximumPeriodMilliseconds  Longest wait between two readiness ticks, in milliseconds. The call passes it to the platform TTY.
     * @param dims              Initial terminal dimensions.
     * @param shell             Shell program name or absolute path.
     * @param args              Space-separated arguments for the shell.
     * @param workingDirectory  Initial working directory for the shell.
     * @param environment       Variables that the shell receives in addition to the environment of the parent.
     * @return                  `true` when the record is complete. `false` when the platform TTY fails to open or the record times out.
     */
    bool open (jam::AsyncUpdater::Context context, int bufferSize, double maximumPeriodMilliseconds, jam::Cell::Rectangle dims, const juce::String& shell,
               const juce::String& args, const juce::String& workingDirectory,
               const juce::StringPairArray& environment) override;

    /** @brief Closes the platform TTY. */
    void close() override;

    /**
     * @brief Reads the next bytes of the record.
     *
     * @param buf       Destination buffer.
     * @param maxBytes  Maximum bytes to read.
     * @return          Bytes read, or -1 when the record has no more bytes or is not ready.
     */
    int read (char* buf, int maxBytes) override;

    /**
     * @brief Accepts input and discards it.
     *
     * @param buf  Data to write. The function does not read it.
     * @param len  Number of bytes.
     * @return     @p len.
     */
    int write (const char* buf, int len) override;

    /** @brief Ignores the window size. */
    void setWinsize (jam::TerminalWinsize ws) override;

private:
    /** @brief Reads all bytes that the platform TTY has into the record. Signals `recorded` when the platform TTY reports the end. */
    void handleAsyncUpdate() override;

    /** @brief Longest time, in ms, that `open()` waits for the record. */
    static constexpr int recordTimeoutMs { 60000 };

    /** @brief Platform TTY that runs the shell. */
    std::unique_ptr<jam::TerminalTTY> tty;

    /** @brief Block that receives one read of the platform TTY. */
    juce::MemoryBlock chunk;

    /** @brief The recorded shell output. */
    juce::MemoryBlock fixture;

    /** @brief Signalled when the platform TTY reports the end of the output. */
    juce::WaitableEvent recorded;

    /** @brief Stream over `fixture` that `read()` serves. It exists after the record is complete. */
    std::unique_ptr<juce::MemoryInputStream> stream;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TerminalMemoryTTY)
};
