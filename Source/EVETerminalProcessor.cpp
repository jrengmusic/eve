#include "EVETerminalProcessor.h"
#include "TerminalMemoryTTY.h"
#include "ShellIntegration.h"

static constexpr int virtualBlockSize { 65536 };
static constexpr int nullTerminatorBytes { 1 };
static constexpr int gridEventBytes { static_cast<int> (sizeof (juce::int32) + sizeof (juce::int64)) };

static juce::String getDefaultShell()
{
#if JUCE_WINDOWS
    return Id::defaultShellWindows;
#else
    return juce::SystemStats::getEnvironmentVariable (Id::shellVariable, Id::defaultShell);
#endif
}

EVETerminalProcessor::EVETerminalProcessor (juce::AudioProcessor& processorToUse, jam::AudioModel& modelToUse)
    : processor (processorToUse),
      model (modelToUse),
      bypass (*model.getRawParameterValue (jam::Format::toTag (Id::bypass))),
      state (model.state.getOrCreateChildWithName (jam::Format::toType (Id::terminal), nullptr))
{
    state.stopTimer();
    jam::ModelLayout::addParameters (state, *jam::MarkdownDocument::getOrCreate (juce::Identifier { files::parametersLayout }));
    registerEvents();
    registerUserInterfaceGetters();
    registerChainEvents();
}

EVETerminalProcessor::~EVETerminalProcessor() = default;

#if JAM_USING_PROFILER
juce::File EVETerminalProcessor::getManifestFile()
{
    return juce::File::getCurrentWorkingDirectory().getChildFile (juce::JUCEApplicationBase::getCommandLineParameterArray().getReference (0));
}
#endif

void EVETerminalProcessor::setState (juce::ValueTree newState)
{
    const auto restored { newState.getChildWithName (terminalTag) };

    if (restored.isValid())
    {
        state.setValuesFrom (restored);
        newState.removeChild (restored, nullptr);
    }

    state.state.getParent().removeChild (state.state, nullptr);
    newState.appendChild (state.state, nullptr);
}

void EVETerminalProcessor::flush()
{
    const juce::MessageManagerLock messageManagerLock;

    state.flush();
}

#if JUCE_WINDOWS
juce::File EVETerminalProcessor::getRuntime (const juce::String& program)
{
    const std::array<juce::File, 2> runtimes
    {{
        juce::File { jam::ParameterManager::getInstance()->getDefaultValueForSettings (jam::Format::toTag (Id::msysRuntime), Id::defaultMsysRuntime) },
        juce::File { jam::ParameterManager::getInstance()->getDefaultValueForSettings (jam::Format::toTag (Id::cygwinRuntime), Id::defaultCygwinRuntime) }
    }};
    const auto runtime { std::find_if (runtimes.begin(), runtimes.end(), [&program] (const juce::File& file)
                                       {
                                           return juce::File::isAbsolutePath (program) and juce::File { program }.getParentDirectory().getChildFile (file.getFileName()).existsAsFile();
                                       }) };

    return runtime != runtimes.end() ? *runtime : juce::File {};
}
#endif

std::unique_ptr<jam::TerminalTTY> EVETerminalProcessor::createTTY (const juce::String& program, const juce::File& directory)
{
#if JUCE_WINDOWS
    const auto runtime { getRuntime (program) };
    std::unique_ptr<jam::TerminalTTY> platformTTY;

    if (runtime.getFullPathName().isNotEmpty())
    {
        platformTTY = std::make_unique<jam::TerminalCygwinTTY> (ShellIntegration::getOrCreateCygwinPty (directory), runtime);
    }
    else
    {
        auto windowsTTY { std::make_unique<jam::TerminalWindowsTTY>() };
        windowsTTY->setInputMode = [this] (int mode) { state.storeValue (terminalTag, Id::inputMode, mode); };
        windowsTTY->setOutputMode = [this] (int mode) { state.storeValue (terminalTag, Id::outputMode, mode); };
        platformTTY = std::move (windowsTTY);
    }
#else
    std::unique_ptr<jam::TerminalTTY> platformTTY { std::make_unique<jam::TerminalUnixTTY>() };
#endif

#if EVE_USING_REPLAY
    platformTTY = std::make_unique<TerminalMemoryTTY> (std::move (platformTTY), static_cast<int> (getProfilerTable()[jam::Format::toType (Id::recordCapacity)]));
#endif

    platformTTY->onBytesReady = [this] (int numSamples)
    {
#if JAM_USING_PROFILER
        const jam::Profiler::Zone zone { Id::deviceClock };
#endif
        device.clock (numSamples);
    };

    return platformTTY;
}

void EVETerminalProcessor::registerEvents()
{
    for (const auto& id : { Id::applicationCursor, Id::bracketedPaste, Id::mouseSgr, Id::cursorVisible, Id::alternateScreen, Id::isRecording, Id::shellExited, Id::syncOutput, Id::autoWrap, Id::originMode, Id::insertMode, Id::isWrapPending, Id::mouseTracking, Id::cursorShape, Id::keyboardFlags, Id::cursor, Id::numPrompts, Id::activeLink, Id::keyboardFlagsInactive, Id::numKeyboardModesNormal, Id::numKeyboardModesAlternate, Id::savedCursorNormal, Id::savedCursorAlternate })
        events.add<int> (id, [this] (const juce::Identifier& key, int value) { state.storeValue (terminalTag, key, value); });

    events.add<const char*, int> (Id::command, [this] (const juce::Identifier&, const char* name, int size)
                                  {
                                      const auto numBytes { size + nullTerminatorBytes };

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
    for (const auto& id : gridEventTypes)
        events.add<int64_t> (id, [this] (const juce::Identifier& key, int64_t line)
                                 {
                                     const auto index { static_cast<int> (std::distance (gridEventTypes.begin(), std::find (gridEventTypes.begin(), gridEventTypes.end(), key))) };

                                     jassert (index < static_cast<int> (gridEventTypes.size()));

                                     if (gridEventBytes <= gridEvents.getFreeSpace())
                                     {
                                         gridEvents.writeInt (index);
                                         gridEvents.writeInt64 (line);
                                         gridEvents.flush();
                                         state.storeValue (terminalTag, Id::numGridEvents, state.getParameter<jam::Parameter<int64_t>> (terminalTag, Id::numGridEvents)->getValue() + 1);
                                     }
                                 });
}

void EVETerminalProcessor::registerImageEvents()
{
    events.add<int64_t> (Id::clipboard, [this] (const juce::Identifier&, int64_t image) { state.storeValue (terminalTag, Id::pendingClipboard, image); });

    for (const auto& id : { Id::previewBox, Id::outputBlock, Id::scrollingRegion, Id::pen, Id::savedPenNormal, Id::savedPenAlternate, Id::keyboardModesNormal, Id::keyboardModesAlternate, Id::syncStart, Id::firstLine, Id::endLine, Id::closedLine, Id::lineIndex, Id::firstImage, Id::endImage, Id::imageIndex })
    {
        jassert (state.getParameter<jam::Parameter<int64_t>> (terminalTag, id) != nullptr);
        auto& parameter { *state.getParameter<jam::Parameter<int64_t>> (terminalTag, id) };

        events.add<int64_t> (id, [&parameter] (const juce::Identifier&, int64_t value) { parameter.store (value); });
    }
}

void EVETerminalProcessor::applyCommands (const std::function<void (const juce::Identifier&)>& callback)
{
    commands.applyPendingData ([&callback] (const char* data, int size)
    {
        const char* const end { data + size };
        const char* name { data };

        while (name < end)
        {
            const char* const terminator { std::find (name, end, static_cast<char> (Chars::nullCharacter)) };

            jassert (terminator != end);
            callback (juce::Identifier { juce::String::CharPointerType (name), juce::String::CharPointerType (terminator) });
            name = terminator + nullTerminatorBytes;
        }

        return size;
    });
}

void EVETerminalProcessor::setGridEvents()
{
    static const juce::Identifier listsType { jam::Format::toType (Id::gridStarts) };

    static const std::array<void (*) (juce::ValueTree&, juce::int64), std::tuple_size_v<decltype (gridEventTypes)>> listUpdates
    {{
        [] (juce::ValueTree& lists, juce::int64 line)
        {
            lists.appendChild (juce::ValueTree { Id::gridStart, { { Id::value, line } } }, nullptr);
            lists.setProperty (Id::activeGridStart, line, nullptr);
        },

        [] (juce::ValueTree& lists, juce::int64 line)
        {
            lists.removeChild (lists.getChildWithProperty (Id::value, line), nullptr);
            lists.setProperty (Id::activeGridStart, lists.getChild (lists.getNumChildren() - 1).getProperty (Id::value, jam::TerminalBuffer::noGridStart), nullptr);
        }
    }};

    auto lists { state.getOrCreateChildWithName (listsType) };

    gridEvents.applyPendingData ([this, &lists] (const char* data, int size)
    {
        for (int offset { 0 }; offset < size; offset += gridEventBytes)
        {
            const auto index { static_cast<size_t> (juce::ByteOrder::littleEndianInt (data + offset)) };
            const auto line { static_cast<juce::int64> (juce::ByteOrder::littleEndianInt64 (data + offset + sizeof (juce::int32))) };

            listUpdates.at (index) (lists, line);
        }

        return size;
    });

    const auto firstLine { static_cast<juce::int64> (state.state.getProperty (Id::firstLine)) };

    while (lists.getNumChildren() > 0 and static_cast<juce::int64> (lists.getChild (0).getProperty (Id::value)) < firstLine)
        lists.removeChild (0, nullptr);

    if (static_cast<juce::int64> (lists.getProperty (Id::activeGridStart, jam::TerminalBuffer::noGridStart)) < firstLine)
        lists.setProperty (Id::activeGridStart, lists.getChild (0).getProperty (Id::value, jam::TerminalBuffer::noGridStart), nullptr);
}

void EVETerminalProcessor::registerUserInterfaceGetters()
{
    userInterfaceGetters[Id::textEditor].add<jam::TerminalSwap*&, juce::AbstractFifo*&, const jam::Model*&> (jam::Format::toType (Id::winsize), [this] (const juce::Identifier&, jam::TerminalSwap*& output, juce::AbstractFifo*& liveSlotsOutput, const jam::Model*& stateOutput)
                                                                                                    {
                                                                                                        state.flush();

                                                                                                        if (not static_cast<bool> (state.state.getProperty (Id::syncOutput)))
                                                                                                            triggerAsyncUpdate();

#if JAM_USING_PROFILER
                                                                                                        if (auto* profiler { jam::Profiler::getInstance() })
                                                                                                            profiler->drain();
#endif

                                                                                                        output = &swap;
                                                                                                        liveSlotsOutput = &liveSlots;
                                                                                                        stateOutput = &state;

                                                                                                        setPendingInput();
                                                                                                    });

    userInterfaceGetters[Id::previewComponent].add<jam::TerminalSwap*&, const jam::Model*&> (jam::Format::toType (Id::images), [this] (const juce::Identifier&, jam::TerminalSwap*& output, const jam::Model*& stateOutput) { output = &swap; stateOutput = &state; });
}

void EVETerminalProcessor::registerChainEvents()
{
    chainEvents[Id::terminalInput].add<const char*, int> (Id::write, [this] (const juce::Identifier&, const char* bytes, int size) { addInput (bytes, size); });
}

void EVETerminalProcessor::addInput (const char* bytes, int size)
{
    const auto numBytes { pendingInput.empty() ? juce::jmin (input.getFreeSpace(), size) : 0 };

    if (numBytes > 0)
    {
        input.write (bytes, static_cast<size_t> (numBytes));
        input.flush();
    }

    pendingInput.append (bytes + numBytes, static_cast<size_t> (size - numBytes));
    setPendingInput();
}

void EVETerminalProcessor::setPendingInput()
{
    const auto numBytes { juce::jmin (input.getFreeSpace(), static_cast<int> (pendingInput.size() - pendingInputOffset)) };

    if (numBytes > 0)
    {
        input.write (pendingInput.data() + pendingInputOffset, static_cast<size_t> (numBytes));
        input.flush();
        pendingInputOffset += static_cast<size_t> (numBytes);

        if (pendingInputOffset == pendingInput.size())
        {
            pendingInput.clear();
            pendingInputOffset = 0;
        }
    }

    device.requestProcess();
}

void EVETerminalProcessor::setOpenCommand (std::string_view command, std::string_view path)
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
    const auto hasImages { images.getNumReady() > 0 };

    return swap.writeFromTerminalBuffer (terminalBuffer, images, events, *state.getRawParameterValue<int64_t> (terminalTag, Id::heldLine), *state.getRawParameterValue<int64_t> (terminalTag, Id::heldImage)) > 0 or hasImages;
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
    const auto heldImage { state.getRawParameterValue<int64_t> (terminalTag, Id::heldImage)->load (std::memory_order_seq_cst) };

    state.storeValue (terminalTag, Id::heldImage, static_cast<int64_t> (heldImage >= 0 ? juce::jmin (heldImage, image) : image));

    swap.setImageData (image, *state.getRawParameterValue<int64_t> (terminalTag, Id::firstImage), *state.getRawParameterValue<int64_t> (terminalTag, Id::endImage), *state.getRawParameterValue<int64_t> (terminalTag, Id::imageIndex), [] (const void* data, size_t size, jam::TerminalImageEncoding)
    {
        juce::SystemClipboard::copyTextToClipboard (juce::String::fromUTF8 (static_cast<const char*> (data), static_cast<int> (size)));
    });

    state.storeValue (terminalTag, Id::heldImage, static_cast<int64_t> (heldImage));
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
    JUCE_ASSERT_MESSAGE_THREAD

    static constexpr double virtualSampleRate { 48000.0 };

    device.stop();

    if (const auto winsize { getWinsize() }; winsize.isValid())
    {
        if (tty == nullptr)
        {
            const auto vblankMilliseconds { juce::RelativeTime::seconds (1.0 / jam::CpuGraphicsEngine::getRefreshRateHz()).inSeconds() * 1000.0 };
            openTTY (vblankMilliseconds, winsize);

            if (tty != nullptr)
            {
                ttyResize.onWork = [this] { tty->setWinsize (getWinsize()); };
                ttyResize.open (device.getContext());
            }
        }

        if (tty != nullptr)
        {
            {
#if JAM_USING_PROFILER
                const jam::Profiler::Zone zone { Id::deviceOpen };
#endif
                device.open ({}, {}, virtualSampleRate, tty->getDefaultBufferSize());
            }
#if JAM_USING_PROFILER
            const jam::Profiler::Zone zone { Id::deviceStart };
#endif
            start();
        }
    }
}

void EVETerminalProcessor::start()
{
    device.start (this);
    device.requestProcess();
}

void EVETerminalProcessor::releaseResources()
{
    device.stop();
    cancelPendingUpdate();
    device.close();
}

void EVETerminalProcessor::audioDeviceIOCallbackWithContext (const float* const*, int, float* const*, int, int, const juce::AudioIODeviceCallbackContext&)
{
    process();
}

void EVETerminalProcessor::audioDeviceAboutToStart (juce::AudioIODevice*)
{
    {
#if JAM_USING_PROFILER
        const jam::Profiler::Zone zone { Id::queuesPrepare };
#endif
        input.prepare (virtualBlockSize);
        commands.prepare (virtualBlockSize);
        gridEvents.prepare (virtualBlockSize);
        terminalProcessor.prepare (virtualBlockSize);
    }

    if (const auto winsize { getWinsize() }; winsize.isValid())
        prepare (winsize, getHistoryCapacity());
}

void EVETerminalProcessor::audioDeviceStopped() {}

void EVETerminalProcessor::openTTY (double maximumPeriodMilliseconds, jam::TerminalWinsize winsize)
{
    const auto directory { jam::ParameterManager::getInstance()->getUserSettings().getParentDirectory().getParentDirectory().getChildFile (files::shellIntegrationDirectory) };

#if JAM_USING_PROFILER
    const jam::Profiler::Zone zone { Id::ttyOpen };

    if (processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
    {
        const auto profilerTable { getProfilerTable() };
        const auto program { profilerTable[jam::Format::toType (Id::shell)].toString() };

        ShellIntegration::addShellScripts (directory);
        tty = createTTY (program, directory);

        const auto isOpen { tty->open (device.getContext(), maximumPeriodMilliseconds, winsize.toCellRect(), program, profilerTable[jam::Format::toType (Id::args)].toString(), getManifestFile().getParentDirectory().getFullPathName()) };

        const auto startUrl { juce::URL { getManifestFile().getParentDirectory() }.toString (false) };

        state.storeValue (terminalTag, Id::cwd, startUrl.toRawUTF8(), static_cast<int> (startUrl.getNumBytesAsUTF8()));
        state.storeValue (terminalTag, Id::shell, program.toRawUTF8(), static_cast<int> (program.getNumBytesAsUTF8()));

        if (not isOpen)
        {
            state.storeValue (terminalTag, Id::shellExited, 1);
            tty.reset();
        }
    }
    else
#endif
    {
        const auto shellTokens (juce::StringArray::fromTokens (getShell(), true));
        const auto program { shellTokens[0].unquoted() };
        const auto arguments { shellTokens.joinIntoString (juce::String::charToString (Chars::space), 1) };

        ShellIntegration::addShellScripts (directory);

        const juce::URL cwd { state.state.getProperty (Id::cwd).toString() };
        const auto startDirectory { cwd.isLocalFile() ? cwd.getLocalFile() : ShellIntegration::getStartDirectory() };
        const auto startUrl { juce::URL { startDirectory }.toString (false) };

        state.storeValue (terminalTag, Id::cwd, startUrl.toRawUTF8(), static_cast<int> (startUrl.getNumBytesAsUTF8()));

        const auto openShell = [this, maximumPeriodMilliseconds, winsize, &directory, &startDirectory] (const juce::String& shell, const juce::String& shellArguments) -> bool
        {
            tty = createTTY (shell, directory);

            return tty->open (device.getContext(), maximumPeriodMilliseconds, winsize.toCellRect(), shell, (shellArguments + juce::String::charToString (Chars::space) + ShellIntegration::getShellArguments (shell, directory)).trim(), startDirectory.getFullPathName(), ShellIntegration::getShellEnvironment (shell, directory));
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
        {
            state.storeValue (terminalTag, Id::shellExited, 1);
            tty.reset();
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
        dynamic_cast<const jam::ENAXClientExtensions&> (processor).applyRule (Id::manifestFile, [] (juce::File&) {}, manifestFile);

    if (const auto document { jam::ConfigDocument::parse (manifestFile.loadFileAsString(), manifestFile.getFullPathName()) }; document != nullptr)
        return document->getValueTree (Id::manifest).getChildWithName (jam::Format::toType (Id::profiler));

    return {};
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
    terminalProcessor.setWinsize (winsize, terminalBuffer);
    handleUpdateNow();

    const auto blocksPerVblank { juce::jmax (1, static_cast<int> (std::ceil (device.getCurrentSampleRate() / virtualBlockSize / jam::CpuGraphicsEngine::getRefreshRateHz()))) };

    {
#if JAM_USING_PROFILER
        const jam::Profiler::Zone zone { Id::bufferPrepare };
#endif
        {
#if JAM_USING_PROFILER
            const jam::Profiler::Zone rowsZone { Id::rowsPrepare };
#endif
            terminalBuffer.prepare (winsize.cols, winsize.rows, blocksPerVblank * virtualBlockSize + winsize.rows);
        }
        {
#if JAM_USING_PROFILER
            const jam::Profiler::Zone swapZone { Id::swapPrepare };
#endif
            swap.prepare (terminalBuffer.getNumBytes(), terminalBuffer.getNumRowBytes());
        }
        {
#if JAM_USING_PROFILER
            const jam::Profiler::Zone resetZone { Id::processorReset };
#endif
            terminalProcessor.reset (terminalBuffer, images, winsize, events);
        }
        swap.setCapacity (historyCapacity, events);
        handleUpdateNow();
    }

    ttyResize.submit();
    flush();
}

void EVETerminalProcessor::process() noexcept
{
    if (juce::exactlyEqual (bypass.load(), 0.0f))
    {
        input.writePendingData (*tty);

        const auto background { state.getParameter<jam::Parameter<int64_t>> (terminalTag, Id::background)->getValue() };

        while (terminalProcessor.process (*tty, terminalBuffer, swap, images, liveSlots, *jam::Colours::getInstance(), static_cast<juce::uint32> (background), events)) {}
    }
}

void EVETerminalProcessor::clearHistory()
{
    JUCE_ASSERT_MESSAGE_THREAD

    device.stop();
    handleUpdateNow();
    terminalProcessor.clear (terminalBuffer);
    prepare();
}