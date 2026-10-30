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

EVETerminalProcessor::EVETerminalProcessor (juce::AudioProcessor& processorToUse, jam::AudioModel& modelToUse)
    : processor (processorToUse),
      model (modelToUse),
      bypass (*model.getRawParameterValue (Id::toTag (Id::bypass))),
      state (model.state.getOrCreateChildWithName (Id::toType (Id::terminal), nullptr))
{
    state.stopTimer();
    jam::ModelLayout::addParameters (state, jam::MarkdownDocument::getOrCreate (juce::Identifier { files::parametersLayout }));
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
    jassert (not device.isPlaying());

    state.removeListener (this);
    cancelPendingUpdate();
}

void EVETerminalProcessor::valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property)
{
    if (property == Id::shellExited and static_cast<bool> (tree.getProperty (property)))
        juce::JUCEApplicationBase::quit();
}

#if JAM_USING_PROFILER
juce::File EVETerminalProcessor::getManifestFile()
{
    return juce::File::getCurrentWorkingDirectory().getChildFile (juce::JUCEApplicationBase::getCommandLineParameterArray().getReference (0));
}
#endif

void EVETerminalProcessor::setState (juce::ValueTree newState)
{
    const auto restored { newState.getChildWithName (Id::toType (Id::terminal)) };

    if (restored.isValid())
    {
        state.replaceState (restored);
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
    for (const auto& id : { Id::applicationCursor, Id::bracketedPaste, Id::mouseSgr, Id::cursorVisible, Id::alternateScreen, Id::isRecording, Id::shellExited })
        events.add<bool> (id, [this, id, tag = Id::toType (Id::terminal)] (bool value) { state.storeValue (tag, id, static_cast<int> (value)); });

    for (const auto& id : { Id::mouseTracking, Id::cursor })
        events.add<int> (id, [this, id, tag = Id::toType (Id::terminal)] (int value) { state.storeValue (tag, id, value); });

    events.add<const char*, int> (Id::command, [this] (const char* name, int size) { addCommand (name, size); });

    events.add<const char*, int> (Id::preview, [this, tag = Id::toType (Id::terminal)] (const char* path, int size) { state.storeValue (tag, Id::preview, path, size); });

    events.add<juce::int64> (Id::previewImage, [this, tag = Id::toType (Id::terminal)] (juce::int64 image)
                             {
                                 const auto previous { state.getParameter<jam::Parameter<int64_t>> (tag, Id::previewImage)->getValue() };

                                 if (previous >= 0)
                                     terminalBuffer.addReleasedImage (previous);

                                 state.storeValue (tag, Id::previewImage, static_cast<int64_t> (image));
                             });
}

void EVETerminalProcessor::addCommand (const char* name, int size)
{
    const auto tag { Id::toType (Id::terminal) };
    const auto numBytes { size + static_cast<int> (sizeof (char)) };

    jassert (numBytes <= commands.getFreeSpace());

    if (size > 0 and numBytes <= commands.getFreeSpace())
    {
        commands.write (name, static_cast<size_t> (size));
        commands.writeByte ('\0');
        commands.flush();
        state.storeValue (tag, Id::commandCount, state.getParameter<jam::Parameter<int>> (tag, Id::commandCount)->getValue() + 1);
    }
}

jam::TerminalFIFO& EVETerminalProcessor::getCommands() noexcept { return commands; }

jam::TerminalSwap& EVETerminalProcessor::getSwap() noexcept { return swap; }

void EVETerminalProcessor::registerUserInterfaceGetters()
{
    userInterfaceGetters[Id::textEditor].add<jam::TerminalSwap*&> (Id::toType (Id::winsize), [this] (jam::TerminalSwap*& output)
                                                                                                    {
                                                                                                        triggerAsyncUpdate();

                                                                                                        state.flush();

#if JAM_USING_PROFILER
                                                                                                        if (auto* profiler { jam::Profiler::getInstance() })
                                                                                                            profiler->drain();
#endif

                                                                                                        output = &swap;

                                                                                                        device.requestProcess();
                                                                                                    });
}

void EVETerminalProcessor::registerChainEvents()
{
    chainEvents[Id::terminalInput].add<const char*, int> ([this] (const char* bytes, int size)
                                                          {
                                                              jassert (size <= input.getFreeSpace());

                                                              if (size <= input.getFreeSpace())
                                                              {
                                                                  input.write (bytes, static_cast<size_t> (size));
                                                                  input.flush();
                                                                  device.requestProcess();
                                                              }
                                                          });
}

bool EVETerminalProcessor::writePendingData()
{
    auto& images { terminalProcessor.getImages() };
    const auto hasImages { images.getNumReady() > 0 };

    return swap.writeFromTerminalBuffer (terminalBuffer, images) > 0 or hasImages;
}

void EVETerminalProcessor::handleAsyncUpdate()
{
    if (writePendingData())
        device.requestProcess();
}

int EVETerminalProcessor::getHistoryCapacity() const
{
    return static_cast<int> (jam::Model::getChildWithID (model.state, Id::toTag (Id::historyCapacity)).getProperty (Id::value));
}

jam::TerminalWinsize EVETerminalProcessor::getWinsize() const
{
    return jam::TerminalWinsize::unpack (static_cast<juce::int64> (model.getOrCreateComponentProperty (Id::toTag (Id::winsize), Id::value).getValue()));
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
    device.start (this);
    device.requestProcess();
}

void EVETerminalProcessor::releaseResources()
{
    device.stop();
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
        const auto manifestFile { getManifestFile() };
        const auto manifest { jam::ConfigDocument::parse (manifestFile.loadFileAsString(), manifestFile.getFullPathName()).getValueTree (Id::manifest) };
        const auto profilerTable { manifest.getChildWithName (Id::toType (Id::profiler)) };

        [[maybe_unused]] const auto isOpen { tty->open (device.getContext(), bufferSize, maximumPeriodMilliseconds, winsize.toCellRect(), profilerTable[Id::shell].toString(), profilerTable[Id::args].toString(), manifestFile.getParentDirectory().getFullPathName()) };
        jassert (isOpen);
        state.addListener (this);
    }
    else
#endif
    {
        const auto shell { juce::SystemStats::getEnvironmentVariable (juce::String { shellVariable.data() }, juce::String { defaultShell.data() }) };
        const auto directory { jam::ParameterManager::getInstance()->getUserSettings().getParentDirectory().getChildFile (files::shellIntegrationDirectory) };
        const auto isIntegrated { ShellIntegration::isShellIntegrated() };

        if (isIntegrated)
            ShellIntegration::addShellScripts (directory);

        [[maybe_unused]] const auto isOpen { tty->open (device.getContext(), bufferSize, maximumPeriodMilliseconds, winsize.toCellRect(), shell, isIntegrated ? ShellIntegration::getShellArguments (shell, directory) : juce::String {}, ShellIntegration::getStartDirectory().getFullPathName(), isIntegrated ? ShellIntegration::getShellEnvironment (shell, directory) : juce::StringPairArray {}) };
        jassert (isOpen);
    }
}

void EVETerminalProcessor::prepare (jam::TerminalWinsize winsize, int historyCapacity)
{
    triggerAsyncUpdate();
    handleUpdateNowIfNeeded();

    const auto blocksPerVblank { juce::jmax (1, static_cast<int> (std::ceil (device.getCurrentSampleRate() / virtualBlockSize / jam::VulkanEngine::getRefreshRateHz()))) };

    terminalBuffer.prepare (winsize.cols, winsize.rows, blocksPerVblank * virtualBlockSize + winsize.rows);
    swap.prepare (winsize);
    terminalProcessor.reset (terminalBuffer, winsize);
    swap.setCapacity (historyCapacity);

    const auto vblankMilliseconds { static_cast<double> (juce::RelativeTime::seconds (1.0 / jam::VulkanEngine::getRefreshRateHz()).inMilliseconds()) };
    openTTY (device.getCurrentBufferSizeSamples(), vblankMilliseconds, winsize);
    tty->setWinsize (winsize);
}

void EVETerminalProcessor::process() noexcept
{
    if (juce::exactlyEqual (bypass.load(), 0.0f))
    {
        input.writePendingData (*tty);

        while (terminalProcessor.process (*tty, terminalBuffer, swap, *jam::Colours::getInstance(), events)) {}
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
