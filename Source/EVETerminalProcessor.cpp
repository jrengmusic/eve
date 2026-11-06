#include "EVETerminalProcessor.h"
#include "TerminalMemoryTTY.h"
#include "ShellIntegration.h"

static constexpr std::string_view shellVariable { "SHELL" };
#if JUCE_WINDOWS
static constexpr std::string_view defaultShell { "powershell.exe" };
#else
static constexpr std::string_view defaultShell { "/bin/zsh" };
#endif
static constexpr int virtualBlockSize { 65536 };

static juce::String getDefaultShell()
{
    return juce::SystemStats::getEnvironmentVariable (juce::String { shellVariable.data() }, juce::String { defaultShell.data() });
}

EVETerminalProcessor::EVETerminalProcessor (juce::AudioProcessor& processorToUse, jam::AudioModel& modelToUse)
    : processor (processorToUse),
      model (modelToUse),
      bypass (*model.getRawParameterValue (jam::Format::toTag (Id::bypass))),
      state (model.state.getOrCreateChildWithName (jam::Format::toType (Id::terminal), nullptr))
{
    state.stopTimer();
    jam::ModelLayout::addParameters (state, jam::MarkdownDocument::getOrCreate (juce::Identifier { files::parametersLayout }));
    winsizeAttachment = std::make_unique<jam::Model::ParameterAttachment> (*state.getParameter<jam::Parameter<int64_t>> (jam::Format::toType (Id::terminal), Id::winsize),
                                                                          [this] (const juce::var&) { restart(); });
    registerEvents();
    registerUserInterfaceGetters();
    registerChainEvents();
    tty->onBytesReady = [this] (int numSamples)
    {
#if JAM_USING_PROFILER
        const jam::Profiler::Zone zone { Id::deviceClock };
#endif
        device.clock (numSamples);
    };
}

EVETerminalProcessor::~EVETerminalProcessor()
{
    releaseResources();
}

#if JAM_USING_PROFILER
juce::File EVETerminalProcessor::getManifestFile()
{
    return juce::File::getCurrentWorkingDirectory().getChildFile (juce::JUCEApplicationBase::getCommandLineParameterArray().getReference (0));
}
#endif

void EVETerminalProcessor::setState (juce::ValueTree newState)
{
    const auto restored { newState.getChildWithName (jam::Format::toType (Id::terminal)) };

    if (restored.isValid())
    {
        state.setValuesFrom (restored);
        newState.removeChild (restored, nullptr);
    }

    state.state.getParent().removeChild (state.state, nullptr);
    newState.appendChild (state.state, nullptr);
}

void EVETerminalProcessor::setRows (const juce::ValueTree& rows)
{
    for (int index { 0 }; index < rows.getNumProperties(); ++index)
    {
        const auto name { rows.getPropertyName (index) };

        state.state.setProperty (name, rows.getProperty (name), nullptr);
    }
}

void EVETerminalProcessor::flush()
{
    const juce::MessageManagerLock messageManagerLock;

    state.flush();
}

std::unique_ptr<jam::TerminalTTY> EVETerminalProcessor::createTTY()
{
#if JUCE_WINDOWS
    std::unique_ptr<jam::TerminalTTY> platformTTY { std::make_unique<jam::TerminalWindowsTTY> (juce::File::getSpecialLocation (juce::File::currentExecutableFile).getParentDirectory()) };
#else
    std::unique_ptr<jam::TerminalTTY> platformTTY { std::make_unique<jam::TerminalUnixTTY>() };
#endif

#if EVE_USING_REPLAY
    return std::make_unique<TerminalMemoryTTY> (std::move (platformTTY));
#else
    return platformTTY;
#endif
}

void EVETerminalProcessor::registerEvents()
{
    for (const auto& id : { Id::applicationCursor, Id::bracketedPaste, Id::mouseSgr, Id::cursorVisible, Id::alternateScreen, Id::isRecording, Id::shellExited, Id::syncOutput, Id::autoWrap, Id::originMode, Id::insertMode, Id::isWrapPending })
        events.add<bool> (id, [this, id, tag = jam::Format::toType (Id::terminal)] (bool value) { state.storeValue (tag, id, static_cast<int> (value)); });

    for (const auto& id : { Id::mouseTracking, Id::cursorShape, Id::keyboardFlags, Id::cursor, Id::numPrompts, Id::activeLink, Id::keyboardFlagsInactive, Id::numKeyboardModesNormal, Id::numKeyboardModesAlternate, Id::savedCursorNormal, Id::savedCursorAlternate })
        events.add<int> (id, [this, id, tag = jam::Format::toType (Id::terminal)] (int value) { state.storeValue (tag, id, value); });

    events.add<const char*, int> (Id::command, [this, tag = jam::Format::toType (Id::terminal)] (const char* name, int size)
                                  {
                                      const auto numBytes { size + 1 };

                                      if (size > 0 and numBytes <= commands.getFreeSpace())
                                      {
                                          commands.write (name, static_cast<size_t> (size));
                                          commands.writeByte (static_cast<char> (Chars::nullCharacter));
                                          commands.flush();
                                          state.storeValue (tag, Id::numCommands, state.getParameter<jam::Parameter<int>> (tag, Id::numCommands)->getValue() + 1);
                                      }
                                  });

    for (const auto& id : { Id::preview, Id::title, Id::foregroundProcess, Id::cwd })
        events.add<const char*, int> (id, [this, id, tag = jam::Format::toType (Id::terminal)] (const char* path, int size) { state.storeValue (tag, id, path, size); });

    registerImageEvents();
}

void EVETerminalProcessor::registerImageEvents()
{
    events.add<juce::int64> (Id::clipboard, [this, tag = jam::Format::toType (Id::terminal)] (juce::int64 image) { state.storeValue (tag, Id::pendingClipboard, static_cast<int64_t> (image)); });

    for (const auto& id : { Id::previewBox, Id::outputBlock, Id::scrollingRegion, Id::pen, Id::savedPenNormal, Id::savedPenAlternate, Id::keyboardModesNormal, Id::keyboardModesAlternate, Id::syncStart })
        events.add<juce::int64> (id, [this, id, tag = jam::Format::toType (Id::terminal)] (juce::int64 value) { state.storeValue (tag, id, static_cast<int64_t> (value)); });
}

void EVETerminalProcessor::applyCommands (const std::function<void (const juce::Identifier&)>& callback)
{
    juce::String cut {};

    commands.applyPendingData ([&callback, &cut] (const char* data, int size)
    {
        const char* const end { data + size };

        while (data < end)
        {
            const char* const terminator { std::find (data, end, static_cast<char> (Chars::nullCharacter)) };

            if (terminator == end)
                cut += juce::String { juce::String::CharPointerType (data), juce::String::CharPointerType (end) };
            else
            {
                const auto name { cut + juce::String { juce::String::CharPointerType (data), juce::String::CharPointerType (terminator) } };

                cut = juce::String {};

                if (name.isNotEmpty())
                    callback (juce::Identifier { name });
            }

            data = juce::jmin (terminator + 1, end);
        }
    });
}

void EVETerminalProcessor::registerUserInterfaceGetters()
{
    userInterfaceGetters[Id::textEditor].add<jam::TerminalSwap*&> (jam::Format::toType (Id::winsize), [this] (jam::TerminalSwap*& output)
                                                                                                    {
                                                                                                        state.flush();

                                                                                                        if (not static_cast<bool> (state.state.getProperty (Id::syncOutput)))
                                                                                                            triggerAsyncUpdate();

#if JAM_USING_PROFILER
                                                                                                        if (auto* profiler { jam::Profiler::getInstance() })
                                                                                                            profiler->drain();
#endif

                                                                                                        output = &swap;

                                                                                                        applyPendingInput();
                                                                                                    });

    userInterfaceGetters[Id::previewComponent].add<jam::TerminalSwap*&> (jam::Format::toType (Id::images), [this] (jam::TerminalSwap*& output) { output = &swap; });
}

void EVETerminalProcessor::registerChainEvents()
{
    chainEvents[Id::terminalInput].add<const char*, int> ([this] (const char* bytes, int size) { addInput (bytes, size); });
}

void EVETerminalProcessor::addInput (const char* bytes, int size)
{
    pendingInput.append (bytes, static_cast<size_t> (size));
    applyPendingInput();
}

void EVETerminalProcessor::applyPendingInput()
{
    const auto numBytes { juce::jmin (input.getFreeSpace(), static_cast<int> (pendingInput.size())) };

    if (numBytes > 0)
    {
        input.write (pendingInput.data(), static_cast<size_t> (numBytes));
        input.flush();
        pendingInput.erase (0, static_cast<size_t> (numBytes));
    }

    device.requestProcess();
}

void EVETerminalProcessor::applyOpenCommand (std::string_view command, std::string_view path)
{
    std::string line { command };

    jam::Format::appendUtf8 (line, Chars::space);
    ShellIntegration::appendPath (line, getShell(), path);
    jam::Format::appendUtf8 (line, Chars::carriageReturn);
    addInput (line.data(), static_cast<int> (line.size()));
}

std::string EVETerminalProcessor::getQuotedPaths (const juce::StringArray& paths) const
{
    const auto shell { getShell() };
    std::string text;

    for (const auto& path : paths)
    {
        if (not text.empty())
            jam::Format::appendUtf8 (text, Chars::space);

        ShellIntegration::appendPath (text, shell, path.toRawUTF8());
    }

    return text;
}

bool EVETerminalProcessor::writePendingData()
{
    auto& images { terminalProcessor.getImages() };
    const auto hasImages { images.getNumReady() > 0 };

    return swap.writeFromTerminalBuffer (terminalBuffer, images) > 0 or hasImages;
}

void EVETerminalProcessor::handleAsyncUpdate()
{
    const auto image { state.getParameter<jam::Parameter<int64_t>> (terminalTag, Id::pendingClipboard)->getValue() };

    if (writePendingData())
        device.requestProcess();

    state.storeValue (terminalTag, Id::clipboard, static_cast<int64_t> (image));
}

void EVETerminalProcessor::copyToClipboard (juce::int64 image)
{
    swap.applyToImageData (image, [] (const void* data, size_t size, jam::TerminalImageEncoding)
    {
        juce::SystemClipboard::copyTextToClipboard (juce::String::fromUTF8 (static_cast<const char*> (data), static_cast<int> (size)));
    });
}

int EVETerminalProcessor::getHistoryCapacity() const
{
    return static_cast<int> (jam::Model::getChildWithID (model.state, jam::Format::toTag (Id::historyCapacity)).getProperty (Id::value));
}

jam::TerminalWinsize EVETerminalProcessor::getWinsize() const
{
    return jam::TerminalWinsize::unpack (static_cast<juce::int64> (state.state.getProperty (Id::winsize)));
}

void EVETerminalProcessor::prepare()
{
    static constexpr double virtualSampleRate { 48000.0 };
#if JUCE_WINDOWS
    static constexpr int virtualBufferSize { virtualBlockSize };
#else
    static constexpr int virtualBufferSize { 1024 };
#endif

    device.stop();
    device.open ({}, {}, virtualSampleRate, virtualBufferSize);
    start();
}

void EVETerminalProcessor::start()
{
    device.start (this);
    device.requestProcess();
}

void EVETerminalProcessor::releaseResources()
{
    device.stop();
    device.close();
}

void EVETerminalProcessor::audioDeviceIOCallbackWithContext (const float* const*, int, float* const*, int, int, const juce::AudioIODeviceCallbackContext&)
{
    process();
}

void EVETerminalProcessor::restart()
{
    restart ([] {});
}

void EVETerminalProcessor::audioDeviceAboutToStart (juce::AudioIODevice*)
{
    input.prepare (virtualBlockSize);
    commands.prepare (virtualBlockSize);
    terminalProcessor.prepare (virtualBlockSize);

    if (const auto winsize { getWinsize() }; winsize.isValid())
        prepare (winsize, getHistoryCapacity());
}

void EVETerminalProcessor::audioDeviceStopped() {}

void EVETerminalProcessor::openTTY (int bufferSize, double maximumPeriodMilliseconds, jam::TerminalWinsize winsize)
{
#if JAM_USING_PROFILER
    if (processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
    {
        const auto profilerTable { getProfilerTable() };
        const auto isOpen { tty->open (device.getContext(), bufferSize, maximumPeriodMilliseconds, winsize.toCellRect(), profilerTable[jam::Format::toType (Id::shell)].toString(), profilerTable[jam::Format::toType (Id::args)].toString(), getManifestFile().getParentDirectory().getFullPathName()) };
        jassert (isOpen);

        if (not isOpen)
            state.storeValue (jam::Format::toType (Id::terminal), Id::shellExited, 1);
    }
    else
#endif
    {
        if (not tty->isOpen())
        {
            const auto shell { getShell() };
            const auto directory { jam::ParameterManager::getInstance()->getUserSettings().getParentDirectory().getChildFile (files::shellIntegrationDirectory) };
            const auto isIntegrated { ShellIntegration::isShellIntegrated() };

            if (isIntegrated)
                ShellIntegration::addShellScripts (directory);

            const auto cwd { state.state.getProperty (Id::cwd).toString() };
            const auto startDirectory { cwd.isEmpty() ? ShellIntegration::getStartDirectory() : juce::File { cwd } };

            const auto isOpen { tty->open (device.getContext(), bufferSize, maximumPeriodMilliseconds, winsize.toCellRect(), shell, isIntegrated ? ShellIntegration::getShellArguments (shell, directory) : juce::String {}, startDirectory.getFullPathName(), isIntegrated ? ShellIntegration::getShellEnvironment (shell, directory) : juce::StringPairArray {}) };
            jassert (isOpen);

            if (not isOpen)
                state.storeValue (jam::Format::toType (Id::terminal), Id::shellExited, 1);
        }
    }
}

#if JAM_USING_PROFILER
juce::ValueTree EVETerminalProcessor::getProfilerTable() const
{
    juce::File manifestFile;

    if (processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
        manifestFile = getManifestFile();
    else
        dynamic_cast<const jam::ENAXClientExtensions&> (processor).set (Id::manifestFile, [] (juce::File&) {}, manifestFile);

    return jam::ConfigDocument::parse (manifestFile.loadFileAsString(), manifestFile.getFullPathName()).getValueTree (Id::manifest).getChildWithName (jam::Format::toType (Id::profiler));
}
#endif

juce::String EVETerminalProcessor::getShell() const
{
#if JAM_USING_PROFILER
    return getProfilerTable().getProperty (jam::Format::toType (Id::shell), getDefaultShell()).toString();
#else
    return getDefaultShell();
#endif
}

void EVETerminalProcessor::prepare (jam::TerminalWinsize winsize, int historyCapacity)
{
    triggerAsyncUpdate();
    handleUpdateNowIfNeeded();

    const auto blocksPerVblank { juce::jmax (1, static_cast<int> (std::ceil (device.getCurrentSampleRate() / virtualBlockSize / jam::CpuGraphicsEngine::getRefreshRateHz()))) };

    terminalBuffer.prepare (winsize.cols, winsize.rows, blocksPerVblank * virtualBlockSize + winsize.rows);
    swap.prepare (winsize);
    terminalProcessor.reset (terminalBuffer, winsize);
    swap.setCapacity (historyCapacity);

    const auto vblankMilliseconds { static_cast<double> (juce::RelativeTime::seconds (1.0 / jam::CpuGraphicsEngine::getRefreshRateHz()).inMilliseconds()) };
    openTTY (device.getCurrentBufferSizeSamples(), vblankMilliseconds, winsize);
    tty->setWinsize (winsize);
}

void EVETerminalProcessor::process() noexcept
{
    if (juce::exactlyEqual (bypass.load(), 0.0f))
    {
        input.writePendingData (*tty);

        const auto background { state.getParameter<jam::Parameter<int64_t>> (terminalTag, Id::background)->getValue() };

        while (terminalProcessor.process (*tty, terminalBuffer, swap, *jam::Colours::getInstance(), static_cast<juce::uint32> (background), events)) {}
    }
}

void EVETerminalProcessor::clear()
{
    restart ([this]
             {
                 triggerAsyncUpdate();
                 handleUpdateNowIfNeeded();
                 terminalProcessor.clear (terminalBuffer);
             });
}
