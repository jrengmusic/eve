#include "Session.h"

Session::Session (const Runtime& sessionRuntime, HANDLE sizeHandle, std::int64_t packedWinsize, int newBufferSize, const juce::StringArray& shellArguments)
    : bufferSize { newBufferSize },
      runtime { sessionRuntime },
      sizePipe { sizeHandle, &CloseHandle },
      arguments { shellArguments },
      startSize { getWinsize (packedWinsize) }
{
    [[maybe_unused]] const auto isCreated { runtime.pthreadCreate (&masterReader, nullptr, [] (void* session) -> void*
    {
        static_cast<Session*> (session)->run();
        return nullptr;
    }, this) };
    jassert (isCreated == 0);
}

Session::~Session()
{
    runtime.pthreadJoin (masterReader, nullptr);
    runtime.close (master);
}

void Session::handleAsyncUpdate()
{
    stdio.applyPendingData ([this] (const char* data, int size)
    {
        const auto numWritten { runtime.write (master, data, static_cast<size_t> (size)) };
        return static_cast<int> (juce::jmax<juce::int64> (0, numWritten));
    });

    applySizes();
}

bool Session::openShell()
{
    static constexpr int setControllingTerminal { 0x540E };
    static constexpr std::array<int, 2> backgroundTerminalSignals { 21, 22 };
    static constexpr std::uintptr_t ignoreAction { 1 };
    static constexpr int getWindowsProcessId { 18 };

    int slave { -1 };
    const auto ptyResult { runtime.openpty (&master, &slave, nullptr, nullptr, &startSize) };

    if (ptyResult == 0)
    {
        const auto cttyResult { runtime.ioctl (slave, setControllingTerminal, 0) };

        if (cttyResult == 0)
        {
            shell = spawnShell (slave);

            if (shell > 0)
            {
                for (const auto backgroundSignal : backgroundTerminalSignals)
                    runtime.signal (backgroundSignal, reinterpret_cast<void (*) (int)> (ignoreAction));

                runtime.setpgid (shell, shell);
                runtime.tcsetpgrp (slave, shell);
                shellProcess.reset (OpenProcess (SYNCHRONIZE, FALSE, static_cast<DWORD> (runtime.cygwinInternal (getWindowsProcessId, shell))));
                jassert (shellProcess != nullptr);
            }
        }

        runtime.close (slave);
    }

    return shell > 0;
}

int Session::spawnShell (int slave) const
{
    static constexpr int noWait { 2 };
    static constexpr std::array<DWORD, 3> standardHandleIds { STD_INPUT_HANDLE, STD_OUTPUT_HANDLE, STD_ERROR_HANDLE };

    std::vector<const char*> shellArguments;
    std::transform (arguments.begin(), arguments.end(), std::back_inserter (shellArguments), std::mem_fn (&juce::String::toRawUTF8));
    shellArguments.push_back (nullptr);

    std::array<HANDLE, standardHandleIds.size()> standardHandles {};
    std::array<int, standardHandleIds.size()> savedFds {};

    for (int fd { 0 }; fd < static_cast<int> (standardHandles.size()); ++fd)
        standardHandles.at (fd) = GetStdHandle (standardHandleIds.at (fd));

    for (int fd { 0 }; fd < static_cast<int> (savedFds.size()); ++fd)
    {
        savedFds.at (fd) = runtime.dup (fd);
        runtime.dup2 (slave, fd);
    }

    const auto pid { runtime.spawnv (noWait, shellArguments.front(), shellArguments.data()) };

    for (int fd { 0 }; fd < static_cast<int> (savedFds.size()); ++fd)
    {
        if (savedFds.at (fd) >= 0)
        {
            runtime.dup2 (savedFds.at (fd), fd);
            runtime.close (savedFds.at (fd));
        }
        else
        {
            runtime.close (fd);
            SetStdHandle (standardHandleIds.at (fd), standardHandles.at (fd));
        }
    }

    return pid;
}

bool Session::openRelays()
{
    stdio.onBytesReady = [this] (int numRead)
    {
        if (numRead < 0)
            juce::MessageManager::callAsync ([this] { applyHangup(); });
        else
            triggerAsyncUpdate();
    };

    sizes.prepare (bufferSize);
    sizeReader.onBytesReady = [this] (int)
    {
        sizes.writeFromTerminalTTY (sizePipe.get());
        triggerAsyncUpdate();
    };

    shellExit.onReady = [this] (int)
    {
        juce::MessageManager::callAsync ([this] { releaseTerminal(); });
    };

    const auto isStdioOpen { stdio.open (nullptr, bufferSize, maximumPeriodMilliseconds) };
    const auto isSizeReaderOpen { sizeReader.open (sizePipe.get(), nullptr, bufferSize, maximumPeriodMilliseconds) };
    const auto isShellExitOpen { shellExit.open (nullptr, shellProcess.get()) };

    if (isShellExitOpen)
        shellExit.arm();

    return isStdioOpen and isSizeReaderOpen and isShellExitOpen;
}

void Session::applyHangup()
{
    static constexpr int hangupSignal { 1 };

    runtime.kill (shell, hangupSignal);
}

void Session::releaseTerminal()
{
    runtime.setpgid (0, shell);
    runtime.setsid();
}

Winsize Session::getWinsize (std::int64_t packed)
{
    const auto unpacked { jam::TerminalWinsize::unpack (packed) };

    return { static_cast<unsigned short> (unpacked.rows), static_cast<unsigned short> (unpacked.cols),
             static_cast<unsigned short> (unpacked.pixelWidth), static_cast<unsigned short> (unpacked.pixelHeight) };
}

void Session::applySizes()
{
    static constexpr int setWindowSize { ('T' << 8) | 2 };
    static constexpr int windowChangeSignal { 28 };

    sizes.applyPendingData ([this] (const char* data, int size)
    {
        int consumed { 0 };

        for (; consumed + static_cast<int> (sizeof (std::int64_t)) <= size; consumed += static_cast<int> (sizeof (std::int64_t)))
        {
            std::int64_t packed { 0 };
            std::memcpy (&packed, data + consumed, sizeof (packed));
            const auto winsize { getWinsize (packed) };
            runtime.ioctl (master, setWindowSize, &winsize);
        }

        if (consumed > 0 and shell > 0)
            runtime.kill (shell, windowChangeSignal);

        return consumed;
    });
}

void Session::run()
{
    static constexpr int setNonBlocking { static_cast<int> (0x8004667eu) };
    static constexpr short dataToRead { 1 };
    static constexpr int noTimeout { -1 };
    static constexpr int wouldBlock { 11 };
    static constexpr int interruptedCall { 4 };
    static constexpr unsigned int numPolled { 1u };

    if (openShell())
    {
        int isNonBlocking { 1 };
        runtime.ioctl (master, setNonBlocking, &isNonBlocking);
        const auto masterPipe { reinterpret_cast<HANDLE> (runtime.getOsfhandle (master)) };

        if (not openRelays())
            applyHangup();

        const auto readMaster = [this, masterPipe] (char* data, int maxBytes)
        {
            int numRead { 0 };
            juce::int64 result { 0 };

            do
            {
                result = runtime.read (master, data + numRead, static_cast<size_t> (maxBytes - numRead));
                numRead += static_cast<int> (juce::jmax<juce::int64> (0, result));

                if (result > 0 and numRead < maxBytes)
                    numRead += juce::jmax (0, jam::TerminalFIFO::read (masterPipe, data + numRead, maxBytes - numRead));
            }
            while (result > 0 and numRead < maxBytes);

            const auto isDrained { result < 0 and *runtime.errorNumber() == wouldBlock };
            return numRead > 0 ? numRead : (isDrained ? 0 : -1);
        };

        Pollfd readiness { master, dataToRead, 0 };

        for (int numRead { 0 }; numRead >= 0;)
        {
            const auto numReady { runtime.poll (&readiness, numPolled, noTimeout) };
            const auto isInterrupted { numReady < 0 and *runtime.errorNumber() == interruptedCall };
            numRead = numReady > 0 ? stdio.writeFromDevice (readMaster) : (isInterrupted ? 0 : -1);
        }
    }

    juce::MessageManager::getInstance()->stopDispatchLoop();
}
