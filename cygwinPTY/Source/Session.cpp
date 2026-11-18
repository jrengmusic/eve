#include "Session.h"

static constexpr int setControllingTerminal { 0x540E };
static constexpr int setWindowSize { ('T' << 8) | 2 };
static constexpr int noWait { 2 };
static constexpr int hangupSignal { 1 };
static constexpr std::array<int, 2> backgroundTerminalSignals { 21, 22 };
static constexpr std::uintptr_t ignoreAction { 1 };
static constexpr int getWindowsProcessId { 18 };
static constexpr std::array<DWORD, 3> standardHandleIds { STD_INPUT_HANDLE, STD_OUTPUT_HANDLE, STD_ERROR_HANDLE };

Session::Session (const Runtime& sessionRuntime, HANDLE sizeHandle, std::int64_t packedWinsize, const juce::StringArray& shellArguments)
    : runtime { sessionRuntime },
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
        return static_cast<int> (juce::jmax (0L, numWritten));
    });

    applySizes();
}

bool Session::openShell()
{
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

        return consumed;
    });
}

void Session::run()
{
    if (openShell())
    {
        if (not openRelays())
            applyHangup();

        const auto readMaster = [this]
        {
            return stdio.writeFromDevice ([this] (char* data, int maxBytes)
            {
                return static_cast<int> (runtime.read (master, data, static_cast<size_t> (maxBytes)));
            });
        };

        auto numRead { readMaster() };

        while (numRead > 0)
            numRead = readMaster();
    }

    juce::MessageManager::getInstance()->stopDispatchLoop();
}
