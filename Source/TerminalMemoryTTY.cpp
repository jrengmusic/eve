#include "TerminalMemoryTTY.h"

TerminalMemoryTTY::TerminalMemoryTTY (std::unique_ptr<jam::TerminalTTY> platformTTY)
    : tty { std::move (platformTTY) }
{
}

bool TerminalMemoryTTY::open (jam::AsyncUpdater::Context context, int bufferSize, double maximumPeriodMilliseconds, jam::Cell::Rectangle dims, const juce::String& shell,
                              const juce::String& args, const juce::String& workingDirectory,
                              const juce::StringPairArray& environment)
{
    jassert (onBytesReady != nullptr);

    if (stream != nullptr) return true;

    chunk.setSize (static_cast<size_t> (bufferSize));
    tty->onBytesReady = [this] (int) { triggerAsyncUpdate(); };

    const auto isOpen { tty->open (getContext(), bufferSize, maximumPeriodMilliseconds, dims, shell, args, workingDirectory, environment) };
    jassert (isOpen);

    const auto isRecorded { isOpen and recorded.wait (recordTimeoutMs) };
    jassert (isRecorded);

    if (isRecorded)
    {
        stream = std::make_unique<juce::MemoryInputStream> (fixture, false);

#if JAM_USING_PROFILER
        if (auto* profiler { jam::Profiler::getInstance() })
        {
            profiler->drain();
            profiler->clear();
        }
#endif

#if JUCE_MAC
        dispatch_async_f (context, this, [] (void* memoryTTY) { static_cast<TerminalMemoryTTY*> (memoryTTY)->onBytesReady (0); });
#endif
    }

    return isRecorded;
}

void TerminalMemoryTTY::close()
{
    tty->close();
}

int TerminalMemoryTTY::read (char* buf, int maxBytes)
{
    int result { -1 };

    if (stream != nullptr and not stream->isExhausted())
        result = stream->read (buf, maxBytes);

    return result;
}

int TerminalMemoryTTY::write (const char*, int len)
{
    return len;
}

void TerminalMemoryTTY::setWinsize (jam::TerminalWinsize)
{
}

void TerminalMemoryTTY::handleAsyncUpdate()
{
    const auto blockSize { static_cast<int> (chunk.getSize()) };
    auto bytesRead { tty->read (static_cast<char*> (chunk.getData()), blockSize) };

    while (bytesRead > 0)
    {
        fixture.append (chunk.getData(), static_cast<size_t> (bytesRead));
        bytesRead = tty->read (static_cast<char*> (chunk.getData()), blockSize);
    }

    if (bytesRead < 0)
        recorded.signal();
}
