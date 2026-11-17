#pragma once
#include <JuceHeader.h>

/**
 * @brief Replay TTY. It records the shell output through the platform TTY, then serves the record from memory.
 *
 * `open()` starts the shell on the platform TTY and waits until the shell closes its output.
 * Each read of the platform TTY lands straight in the record ring, `fixture`, so the record has no staging copy.
 * The `onBytesReady` of the platform TTY ignores its `numSamples` and starts its own update.
 * After the record is complete, `read()` serves it from the ring. `read()` returns -1 at the end.
 * The TTY ignores input and window size changes.
 */
class TerminalMemoryTTY : public jam::TerminalTTY,
                          private jam::AsyncUpdater
{
public:
    /**
     * @brief Takes the platform TTY that records the shell output, and sizes the record.
     *
     * The record allocates its ring once, here, so the recording never grows or copies it.
     *
     * @param platformTTY     TTY that runs the shell. The object owns it.
     * @param recordCapacity  Smallest number of bytes that the record holds. A record that fills the ring asserts.
     */
    explicit TerminalMemoryTTY (std::unique_ptr<jam::TerminalTTY> platformTTY, int recordCapacity);

    /**
     * @brief Opens the platform TTY, records its output, then starts the replay.
     *
     * The call blocks until the platform TTY reports the end of the shell output, for at most `recordTimeoutMs`.
     * With the profiler enabled, it then drains and clears the profiler, so the profile holds the replay only.
     * The call starts no tick. The owner of the TTY starts the first tick of the replay.
     * `onBytesReady` must be set (jassert). When the record is complete, the call changes nothing and returns `true`.
     *
     * @param context           Not used. The platform TTY runs its readiness source on the context of this object.
     * @param bufferSize        Device buffer size. The call passes it to the platform TTY.
     * @param maximumPeriodMilliseconds  Longest wait between two readiness ticks, in milliseconds. The call passes it to the platform TTY.
     * @param dims              Initial terminal dimensions.
     * @param shell             Shell program name or absolute path.
     * @param args              Space-separated arguments for the shell.
     * @param workingDirectory  Initial working directory for the shell.
     * @param environment       Variables that the shell receives in addition to the environment of the parent.
     * @return                  `true` when the record is complete. `false` when the platform TTY fails to open or the record times out.
     */
    bool open (const jam::AsyncUpdater* context, int bufferSize, double maximumPeriodMilliseconds, jam::Cell::Rectangle dims, const juce::String& shell,
               const juce::String& args, const juce::String& workingDirectory,
               const juce::StringPairArray& environment) override;

    /** @brief Closes the platform TTY. */
    void close() override;

    bool isOpen() const noexcept override;

    /**
     * @brief Copies the next bytes of the record into @p buf.
     *
     * @param buf       Destination buffer.
     * @param maxBytes  Maximum bytes to read.
     * @return          Bytes read, or -1 when the record has no more bytes.
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

    /** @brief The recorded shell output, in one ring that the constructor sizes. */
    jam::TerminalFIFO fixture;

    /** @brief Signalled when the platform TTY reports the end of the output. It resets manually, so `isOpen()` can test it. */
    juce::WaitableEvent recorded { true };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TerminalMemoryTTY)
};