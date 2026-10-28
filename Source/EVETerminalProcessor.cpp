#include "EVETerminalProcessor.h"
#include "TerminalMemoryTTY.h"
#include "ShellIntegration.h"

static constexpr std::string_view shellVariable { "SHELL" };
#if JUCE_WINDOWS
static constexpr std::string_view defaultShell { "powershell.exe" };
#else
static constexpr std::string_view defaultShell { "/bin/zsh" };
#endif

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
    tty->onBytesReady = [this] { if (onBytesReady != nullptr) onBytesReady(); };
}

EVETerminalProcessor::~EVETerminalProcessor()
{
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
}

void EVETerminalProcessor::registerUserInterfaceGetters()
{
    userInterfaceGetters[Id::textEditor].add<const jam::TerminalSwap*&> (Id::toType (Id::winsize), [this] (const jam::TerminalSwap*& output)
                                                                                                    {
                                                                                                        writePendingInput();

                                                                                                        if (processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
                                                                                                            triggerAsyncUpdate();
                                                                                                        else
                                                                                                            juce::ignoreUnused (writePendingData());

                                                                                                        state.flush();

#if JAM_USING_PROFILER
                                                                                                        if (auto* profiler { jam::Profiler::getInstance() })
                                                                                                            profiler->drain();
#endif

                                                                                                        output = &swap;

                                                                                                        if (onProcessRequest != nullptr)
                                                                                                            onProcessRequest();
                                                                                                    });
}

void EVETerminalProcessor::registerChainEvents()
{
    chainEvents[Id::terminalInput].add<const char*, int> ([this] (const char* bytes, int size)
                                                          {
                                                              pendingInput.append (bytes, static_cast<size_t> (size));
                                                              writePendingInput();
                                                          });
}

void EVETerminalProcessor::writePendingInput()
{
    const auto written { juce::jmin (static_cast<int> (pendingInput.getSize()), input.getFreeSpace()) };

    input.write (pendingInput.getData(), static_cast<size_t> (written));
    input.flush();

    if (written > 0 and onProcessRequest != nullptr)
        onProcessRequest();
    pendingInput.removeSection (0, static_cast<size_t> (written));
}

bool EVETerminalProcessor::writePendingData()
{
    return swap.writeFromTerminalBuffer (terminalBuffer) > 0;
}

void EVETerminalProcessor::handleAsyncUpdate()
{
    if (writePendingData() and onProcessRequest != nullptr)
        onProcessRequest();
}

int EVETerminalProcessor::getHistoryCapacity() const
{
    return static_cast<int> (jam::Model::getChildWithID (model.state, Id::toTag (Id::historyCapacity)).getProperty (Id::value));
}

jam::TerminalWinsize EVETerminalProcessor::getWinsize() const
{
    return jam::TerminalWinsize::unpack (static_cast<juce::int64> (model.getOrCreateComponentProperty (Id::toTag (Id::winsize), Id::value).getValue()));
}

void EVETerminalProcessor::prepare (const juce::dsp::ProcessSpec& spec, jam::AsyncUpdater::Context context)
{
    jassert (spec.maximumBlockSize > 0);

    input.prepare (static_cast<int> (spec.maximumBlockSize));
    terminalProcessor.prepare (static_cast<int> (spec.maximumBlockSize));

    if (const auto winsize { getWinsize() }; winsize.isValid())
        prepare (spec, winsize, getHistoryCapacity(), context);
}

void EVETerminalProcessor::openTTY (int blockSize, jam::TerminalWinsize winsize, jam::AsyncUpdater::Context context)
{
#if JAM_USING_PROFILER
    if (processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
    {
        const auto manifestFile { getManifestFile() };
        const auto manifest { jam::ConfigDocument::parse (manifestFile.loadFileAsString(), manifestFile.getFullPathName()).getValueTree (Id::manifest) };
        const auto profilerTable { manifest.getChildWithName (Id::toType (Id::profiler)) };

        [[maybe_unused]] const auto isOpen { tty->open (context, blockSize, winsize.toCellRect(), profilerTable[Id::shell].toString(), profilerTable[Id::args].toString(), manifestFile.getParentDirectory().getFullPathName()) };
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

        [[maybe_unused]] const auto isOpen { tty->open (context, blockSize, winsize.toCellRect(), shell, isIntegrated ? ShellIntegration::getShellArguments (shell, directory) : juce::String {}, ShellIntegration::getStartDirectory().getFullPathName(), isIntegrated ? ShellIntegration::getShellEnvironment (shell, directory) : juce::StringPairArray {}) };
        jassert (isOpen);
    }
}

void EVETerminalProcessor::prepare (const juce::dsp::ProcessSpec& spec, jam::TerminalWinsize winsize, int historyCapacity, jam::AsyncUpdater::Context context)
{
    const auto blockSize { static_cast<int> (spec.maximumBlockSize) };

    triggerAsyncUpdate();
    handleUpdateNowIfNeeded();

    const auto blocksPerVblank { juce::jmax (1, static_cast<int> (std::ceil (spec.sampleRate / blockSize / jam::VulkanEngine::getRefreshRateHz()))) };

    terminalBuffer.prepare (winsize.cols, winsize.rows, blocksPerVblank * blockSize);
    swap.prepare (winsize);
    terminalProcessor.reset (terminalBuffer);
    swap.setCapacity (historyCapacity);

    openTTY (blockSize, winsize, context);
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
    processor.suspendProcessing (true);
    triggerAsyncUpdate();
    handleUpdateNowIfNeeded();
    terminalProcessor.clear (terminalBuffer);
    processor.suspendProcessing (false);

    if (onProcessRequest != nullptr)
        onProcessRequest();
}
