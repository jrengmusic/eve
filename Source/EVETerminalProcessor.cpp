#include "EVETerminalProcessor.h"
#include "TerminalMemoryTTY.h"
#include "ShellIntegration.h"

static constexpr int virtualBlockSize { 65536 };
static constexpr int nullTerminatorBytes { 1 };

static juce::String getDefaultShell()
{
#if JUCE_WINDOWS
    return Id::defaultShellWindows;
#else
    return juce::SystemStats::getEnvironmentVariable (Id::shellVariable, Id::defaultShell);
#endif
}

#if JUCE_WINDOWS
static void applySidecar (const juce::File& directory, const juce::String& fileName)
{
    const BinaryData::Raw sidecar { fileName };
    jassert (sidecar.exists());
    const auto file { directory.getChildFile (fileName) };
    juce::MemoryBlock content;
    const auto isMatching { file.loadFileAsData (content) and content.matches (sidecar.data, static_cast<size_t> (sidecar.size)) };

    if (not isMatching)
        file.replaceWithData (sidecar.data, static_cast<size_t> (sidecar.size));
}
#endif

static void applyNames (jam::TerminalFIFO& lane, const std::function<void (const juce::String&)>& callback)
{
    juce::String cut {};

    lane.applyPendingData ([&callback, &cut] (const char* data, int size)
    {
        const char* const end { data + size };

        while (data < end)
        {
            const char* const terminator { std::find (data, end, static_cast<char> (Chars::nullCharacter)) };

            if (terminator == end)
            {
                cut += juce::String { juce::String::CharPointerType (data), juce::String::CharPointerType (end) };
            }
            else
            {
                const auto name { cut + juce::String { juce::String::CharPointerType (data), juce::String::CharPointerType (terminator) } };

                cut = juce::String {};
                callback (name);
            }

            data = juce::jmin (terminator + 1, end);
        }
    });
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
                                                                          [this] (const juce::var&)
                                                                          {
                                                                              restart();
                                                                          });
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
    const auto restored { newState.getChildWithName (jam::Format::toType (Id::pluginState)) };

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
    state.setValuesFrom (rows);
}

void EVETerminalProcessor::flush()
{
    const juce::MessageManagerLock messageManagerLock;

    state.flush();
}

std::unique_ptr<jam::TerminalTTY> EVETerminalProcessor::createTTY()
{
#if JUCE_WINDOWS
    const auto conptyDirectory { jam::File::getOrCreateDirectory (jam::ParameterManager::getInstance()->getUserSettings().getParentDirectory().getParentDirectory(), files::conptyDirectory) };
    applySidecar (conptyDirectory, files::conptyLibrary);
    applySidecar (conptyDirectory, files::openConsole);
    std::unique_ptr<jam::TerminalTTY> platformTTY { std::make_unique<jam::TerminalWindowsTTY> (conptyDirectory) };
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
    for (const auto& id : { Id::applicationCursor, Id::bracketedPaste, Id::mouseSgr, Id::cursorVisible, Id::alternateScreen, Id::isRecording, Id::shellExited, Id::syncOutput, Id::autoWrap, Id::originMode, Id::insertMode, Id::isWrapPending, Id::mouseTracking, Id::cursorShape, Id::keyboardFlags, Id::cursor, Id::numPrompts, Id::activeLink, Id::keyboardFlagsInactive, Id::numKeyboardModesNormal, Id::numKeyboardModesAlternate, Id::savedCursorNormal, Id::savedCursorAlternate })
        events.add<int> (id, [this] (const juce::Identifier& key, int value) { state.storeValue (terminalTag, key, value); });

    events.add<const char*, int> (Id::command, [this] (const juce::Identifier&, const char* name, int size)
                                  {
                                      const auto numBytes { size + nullTerminatorBytes };

                                      jassert (numBytes <= commands.getFreeSpace());

                                      if (size > 0 and numBytes <= commands.getFreeSpace())
                                      {
                                          commands.write (name, static_cast<size_t> (size));
                                          commands.writeByte (static_cast<char> (Chars::nullCharacter));
                                          commands.flush();
                                          state.storeValue (terminalTag, Id::numCommands, state.getParameter<jam::Parameter<int>> (terminalTag, Id::numCommands)->getValue() + 1);
                                      }
                                  });

    for (const auto& id : { Id::preview, Id::title, Id::foregroundProcess, Id::cwd })
        events.add<const char*, int> (id, [this] (const juce::Identifier& key, const char* path, int size) { state.storeValue (terminalTag, key, path, size); });

    registerImageEvents();
    registerGridEvents();
}

void EVETerminalProcessor::registerGridEvents()
{
    for (const auto& id : { Id::gridStart, Id::removedGridStart, Id::firstLine })
        events.add<int64_t> (id, [this] (const juce::Identifier& key, int64_t line)
                                 {
                                     const juce::String value { line };
                                     const auto numBytes { static_cast<int> (key.toString().getNumBytesAsUTF8()) + nullTerminatorBytes + static_cast<int> (value.getNumBytesAsUTF8()) + nullTerminatorBytes };

                                     jassert (numBytes <= gridEvents.getFreeSpace());

                                     if (numBytes <= gridEvents.getFreeSpace())
                                     {
                                         gridEvents.writeString (key.toString());
                                         gridEvents.writeString (value);
                                         gridEvents.flush();
                                         state.storeValue (terminalTag, Id::numGridEvents, state.getParameter<jam::Parameter<int64_t>> (terminalTag, Id::numGridEvents)->getValue() + 1);
                                     }
                                 });
}

void EVETerminalProcessor::registerImageEvents()
{
    events.add<int64_t> (Id::clipboard, [this] (const juce::Identifier&, int64_t image) { state.storeValue (terminalTag, Id::pendingClipboard, image); });

    for (const auto& id : { Id::previewBox, Id::outputBlock, Id::scrollingRegion, Id::pen, Id::savedPenNormal, Id::savedPenAlternate, Id::keyboardModesNormal, Id::keyboardModesAlternate, Id::syncStart })
        events.add<int64_t> (id, [this] (const juce::Identifier& key, int64_t value) { state.storeValue (terminalTag, key, value); });
}

void EVETerminalProcessor::applyCommands (const std::function<void (const juce::Identifier&)>& callback)
{
    applyNames (commands, [&callback] (const juce::String& name)
    {
        if (name.isNotEmpty())
            callback (juce::Identifier { name });
    });
}

void EVETerminalProcessor::applyGridEvents()
{
    static const juce::Identifier listsType { jam::Format::toType (Id::gridStarts) };

    static const auto listUpdates (
        []
        {
            jam::Function::Map<juce::Identifier, jam::Function::pointerCapacity, void (juce::ValueTree&, juce::int64)> rows;

            rows.add (Id::gridStart, [] (const juce::Identifier&, juce::ValueTree& lists, juce::int64 line)
            {
                lists.appendChild (juce::ValueTree { Id::gridStart, { { Id::value, line } } }, nullptr);
                lists.setProperty (Id::activeGridStart, line, nullptr);
            });

            rows.add (Id::removedGridStart, [] (const juce::Identifier&, juce::ValueTree& lists, juce::int64 line)
            {
                lists.removeChild (lists.getChildWithProperty (Id::value, line), nullptr);
                lists.setProperty (Id::activeGridStart, lists.getChild (lists.getNumChildren() - 1).getProperty (Id::value, jam::TerminalBuffer::noGridStart), nullptr);
            });

            rows.add (Id::firstLine, [] (const juce::Identifier&, juce::ValueTree& lists, juce::int64 firstLine)
            {
                while (lists.getNumChildren() > 0 and static_cast<juce::int64> (lists.getChild (0).getProperty (Id::value)) < firstLine)
                    lists.removeChild (0, nullptr);

                if (static_cast<juce::int64> (lists.getProperty (Id::activeGridStart, jam::TerminalBuffer::noGridStart)) < firstLine)
                    lists.setProperty (Id::activeGridStart, lists.getChild (0).getProperty (Id::value, jam::TerminalBuffer::noGridStart), nullptr);
            });

            return rows;
        }()
    );

    auto lists { state.getOrCreateChildWithName (listsType) };
    juce::Identifier event {};

    applyNames (gridEvents, [&lists, &event] (const juce::String& name)
    {
        if (event.isNull())
        {
            event = juce::Identifier { name };
        }
        else
        {
            listUpdates.get (event, lists, juce::int64 { name.getLargeIntValue() });
            event = juce::Identifier {};
        }
    });
}

void EVETerminalProcessor::registerUserInterfaceGetters()
{
    userInterfaceGetters[Id::textEditor].add<jam::TerminalSwap*&> (jam::Format::toType (Id::winsize), [this] (const juce::Identifier&, jam::TerminalSwap*& output)
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

    userInterfaceGetters[Id::previewComponent].add<jam::TerminalSwap*&> (jam::Format::toType (Id::images), [this] (const juce::Identifier&, jam::TerminalSwap*& output) { output = &swap; });
}

void EVETerminalProcessor::registerChainEvents()
{
    chainEvents[Id::terminalInput].add<const char*, int> (Id::write, [this] (const juce::Identifier&, const char* bytes, int size) { addInput (bytes, size); });
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
    ShellIntegration::appendPath (line, state.state.getProperty (Id::shell).toString(), path);
    jam::Format::appendUtf8 (line, Chars::carriageReturn);
    addInput (line.data(), static_cast<int> (line.size()));
}

std::string EVETerminalProcessor::getQuotedPaths (const juce::StringArray& paths) const
{
    const auto shell { state.state.getProperty (Id::shell).toString() };
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

    return swap.writeFromTerminalBuffer (terminalBuffer, images, events) > 0 or hasImages;
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
    return jam::TerminalWinsize::unpack (static_cast<juce::int64> (state.getParameter<jam::Parameter<int64_t>> (terminalTag, Id::winsize)->getValue()));
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
    gridEvents.prepare (virtualBlockSize);
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
        const auto program { profilerTable[jam::Format::toType (Id::shell)].toString() };
        const auto isOpen { tty->open (device.getContext(), bufferSize, maximumPeriodMilliseconds, winsize.toCellRect(), program, profilerTable[jam::Format::toType (Id::args)].toString(), getManifestFile().getParentDirectory().getFullPathName()) };

        state.storeValue (terminalTag, Id::shell, program.toRawUTF8(), static_cast<int> (program.getNumBytesAsUTF8()));

        if (not isOpen)
            state.storeValue (terminalTag, Id::shellExited, 1);
    }
    else
#endif
    {
        if (not tty->isOpen())
        {
            const auto shellTokens (juce::StringArray::fromTokens (getShell(), true));
            const auto program { shellTokens[0].unquoted() };
            const auto arguments { shellTokens.joinIntoString (juce::String::charToString (Chars::space), 1) };
            const auto directory { jam::ParameterManager::getInstance()->getUserSettings().getParentDirectory().getParentDirectory().getChildFile (files::shellIntegrationDirectory) };

            ShellIntegration::addShellScripts (directory);

            const auto cwd { state.state.getProperty (Id::cwd).toString() };
            const auto startDirectory { juce::File::isAbsolutePath (cwd) ? juce::File { cwd } : ShellIntegration::getStartDirectory() };

            const auto openShell = [this, bufferSize, maximumPeriodMilliseconds, winsize, &directory, &startDirectory] (const juce::String& shell, const juce::String& shellArguments) -> bool
            {
                return tty->open (device.getContext(), bufferSize, maximumPeriodMilliseconds, winsize.toCellRect(), shell, (shellArguments + juce::String::charToString (Chars::space) + ShellIntegration::getShellArguments (shell, directory)).trim(), startDirectory.getFullPathName(), ShellIntegration::getShellEnvironment (shell, directory));
            };

            const auto isOpen { openShell (program, arguments) };
#if JUCE_WINDOWS
            const std::array<juce::String, 2> fallbackShells { Id::powershellProgram, Id::commandPromptProgram };
#else
            const std::array<juce::String, 0> fallbackShells {};
#endif
            const auto fallbackShell { isOpen ? fallbackShells.end() : std::find_if (fallbackShells.begin(), fallbackShells.end(), [&openShell] (const juce::String& shellName) { return openShell (shellName, {}); }) };
            const auto isFallbackOpen { fallbackShell != fallbackShells.end() };
            const auto runningShell { isFallbackOpen ? *fallbackShell : program };
            const auto missingShell { isFallbackOpen ? program : juce::String {} };

            state.storeValue (terminalTag, Id::shell, runningShell.toRawUTF8(), static_cast<int> (runningShell.getNumBytesAsUTF8()));
            state.storeValue (terminalTag, Id::shellNotFound, missingShell.toRawUTF8(), static_cast<int> (missingShell.getNumBytesAsUTF8()));

            if (not isOpen and not isFallbackOpen)
                state.storeValue (terminalTag, Id::shellExited, 1);
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
    return jam::ParameterManager::getInstance()->getDefaultValueForSettings (jam::Format::toTag (Id::shell), getDefaultShell());
#endif
}

void EVETerminalProcessor::prepare (jam::TerminalWinsize winsize, int historyCapacity)
{
    handleUpdateNow();
    terminalProcessor.applyWinsize (winsize, terminalBuffer);
    handleUpdateNow();

    const auto blocksPerVblank { juce::jmax (1, static_cast<int> (std::ceil (device.getCurrentSampleRate() / virtualBlockSize / jam::CpuGraphicsEngine::getRefreshRateHz()))) };

    terminalBuffer.prepare (winsize.cols, winsize.rows, blocksPerVblank * virtualBlockSize + winsize.rows);
    swap.prepare (terminalBuffer.getNumBytes());
    terminalProcessor.reset (terminalBuffer, winsize);
    swap.setCapacity (historyCapacity);
    handleUpdateNow();

    const auto vblankMilliseconds { juce::RelativeTime::seconds (1.0 / jam::CpuGraphicsEngine::getRefreshRateHz()).inSeconds() * 1000.0 };
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

void EVETerminalProcessor::clearHistory()
{
    restart ([this]
             {
                 handleUpdateNow();
                 terminalProcessor.clear (terminalBuffer);
             });
}
