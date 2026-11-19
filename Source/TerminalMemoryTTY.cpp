#include "TerminalMemoryTTY.h"

TerminalMemoryTTY::TerminalMemoryTTY (std::unique_ptr<jam::TerminalTTY> platformTTY, int recordCapacity)
    : tty { std::move (platformTTY) }
{
    fixture.prepare (recordCapacity);
}

TerminalMemoryTTY::~TerminalMemoryTTY()
{
    close();
    cancelPendingUpdate();
}

bool TerminalMemoryTTY::open (const jam::AsyncUpdater*, double maximumPeriodMilliseconds, jam::Cell::Rectangle dims, const juce::String& shell,
                              const juce::String& args, const juce::String& workingDirectory,
                              const juce::StringPairArray& environment)
{
    jassert (onBytesReady != nullptr);

    if (isOpen()) return true;

    tty->onBytesReady = [this] (int) { triggerAsyncUpdate(); };

    const auto isOpen { tty->open (getContext(), maximumPeriodMilliseconds, dims, shell, args, workingDirectory, environment) };
    jassert (isOpen);

    const auto isRecorded { isOpen and recorded.wait (recordTimeoutMs) };
    jassert (isRecorded);

    if (isRecorded)
    {
#if JAM_USING_PROFILER
        if (auto* profiler { jam::Profiler::getInstance() })
        {
            profiler->drain();
            profiler->clear();
        }
#endif
    }

    return isRecorded;
}

bool TerminalMemoryTTY::isOpen() const noexcept
{
    return recorded.wait (0);
}

int TerminalMemoryTTY::getDefaultBufferSize() const noexcept
{
    return tty->getDefaultBufferSize();
}

void TerminalMemoryTTY::close()
{
    tty->close();
    recorded.reset();
}

int TerminalMemoryTTY::read (char* buf, int maxBytes)
{
    int result { -1 };

    fixture.applyPendingData ([buf, maxBytes, &result] (const char* data, int size)
    {
        result = juce::jmin (size, maxBytes);
        std::memcpy (buf, data, static_cast<size_t> (result));

        return result;
    });

    return result;
}

int TerminalMemoryTTY::write (const char*, int len)
{
    return len;
}

void TerminalMemoryTTY::setWinsize (jam::TerminalWinsize)
{
}

void TerminalMemoryTTY::setCursor (jam::Cell::Point newCursor)
{
    tty->setCursor (newCursor);
}

void TerminalMemoryTTY::setPen (int64_t newPen)
{
    tty->setPen (newPen);
}

void TerminalMemoryTTY::handleAsyncUpdate()
{
    auto bytesRead { fixture.writeFromTerminalTTY (*tty) };

    while (bytesRead > 0)
    {
        jassert (fixture.getFreeSpace() > 0);
        bytesRead = fixture.writeFromTerminalTTY (*tty);
    }

    if (bytesRead < 0)
        recorded.signal();
}