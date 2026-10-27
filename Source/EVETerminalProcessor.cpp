#include "EVETerminalProcessor.h"

static constexpr std::string_view shellVariable { "SHELL" };
#if JUCE_WINDOWS
static constexpr std::string_view defaultShell { "powershell.exe" };
#else
static constexpr std::string_view defaultShell { "/bin/zsh" };
#endif
static constexpr std::string_view zshDirectory { "zsh" };
static constexpr std::string_view zshIntegrationFile { "eve-integration" };
static constexpr std::string_view zshEnvironmentFile { ".zshenv" };
static constexpr std::string_view bashIntegrationFile { "bash_integration.bash" };
static constexpr std::string_view powershellIntegrationFile { "powershell_integration.ps1" };

EVETerminalProcessor::EVETerminalProcessor (juce::AudioProcessor& processorToUse, jam::AudioModel& modelToUse)
    : processor (processorToUse), model (modelToUse)
{
    registerParameterSetters();
    registerEvents();
    registerUserInterfaceGetters();
    registerChainEvents();
    tty->onBytesReady = [this] { if (onProcessRequest != nullptr) onProcessRequest(); };
    model.addListener (*this);
}

EVETerminalProcessor::~EVETerminalProcessor()
{
    cancelPendingUpdate();
}

std::unique_ptr<jam::TerminalTTY> EVETerminalProcessor::createTTY()
{
#if JUCE_WINDOWS
    return std::make_unique<jam::TerminalWindowsTTY> (juce::File::getSpecialLocation (juce::File::currentExecutableFile).getParentDirectory());
#else
    return std::make_unique<jam::TerminalUnixTTY>();
#endif
}

void EVETerminalProcessor::registerParameterSetters()
{
    parameters.add<double> (Id::toTag (Id::bypass), [this] (double newValue) { isBypassed = not juce::exactlyEqual (newValue, 0.0); });
}

void EVETerminalProcessor::registerEvents()
{
    for (const auto& id : { Id::applicationCursor, Id::bracketedPaste, Id::mouseSgr, Id::cursorVisible, Id::alternateScreen, Id::isRecording })
    {
        jassert (model.getParameter (Id::toTag (id)) != nullptr);
        events.add<bool> (id, [this, parameter = model.getParameter (Id::toTag (id))] (bool value) { setParameterValue (*parameter, static_cast<float> (value)); });
    }

    for (const auto& id : { Id::mouseTracking, Id::cursorLine, Id::cursorColumn })
    {
        jassert (model.getParameter (Id::toTag (id)) != nullptr);
        events.add<int> (id, [this, parameter = model.getParameter (Id::toTag (id))] (int value) { setParameterValue (*parameter, static_cast<float> (value)); });
    }
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

void EVETerminalProcessor::setParameterValue (juce::RangedAudioParameter& parameter, float value)
{
    const auto normalisedValue { parameter.convertTo0to1 (value) };

    if (not juce::exactlyEqual (parameter.getValue(), normalisedValue))
        parameter.setValueNotifyingHost (normalisedValue);
}

void EVETerminalProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (jam::Map::contains (parameters, parameterID))
        parameters.get (parameterID, static_cast<double> (newValue));
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

bool EVETerminalProcessor::isShellIntegrated() const
{
    const auto text { jam::ParameterManager::getInstance()->getDefaultValueForSettings (Id::toTag (Id::shellIntegration), Id::tokenTrue.toString()) };

    return juce::Identifier { text } == Id::tokenTrue;
}

juce::File EVETerminalProcessor::getStartDirectory() const
{
    return juce::File { jam::ParameterManager::getInstance()->getDefaultValueForSettings (Id::toTag (Id::startDirectory), "~") };
}

void EVETerminalProcessor::addShellScripts (const juce::File& directory)
{
    const auto zshFolder { directory.getChildFile (juce::String { zshDirectory.data() }) };
    const BinaryData::Raw zshEnvironment { files::zshEnvironment };
    const BinaryData::Raw zshIntegration { files::zshIntegration };
    const BinaryData::Raw bashIntegration { files::bashIntegration };
    const BinaryData::Raw powershellIntegration { files::powershellIntegration };

    directory.createDirectory();
    zshFolder.createDirectory();
    zshFolder.getChildFile (juce::String { zshEnvironmentFile.data() }).replaceWithData (zshEnvironment.data, static_cast<size_t> (zshEnvironment.size));
    zshFolder.getChildFile (juce::String { zshIntegrationFile.data() }).replaceWithData (zshIntegration.data, static_cast<size_t> (zshIntegration.size));
    directory.getChildFile (juce::String { bashIntegrationFile.data() }).replaceWithData (bashIntegration.data, static_cast<size_t> (bashIntegration.size));
    directory.getChildFile (juce::String { powershellIntegrationFile.data() }).replaceWithData (powershellIntegration.data, static_cast<size_t> (powershellIntegration.size));
}

juce::StringPairArray EVETerminalProcessor::getShellEnvironment (const juce::String& shell, const juce::File& directory)
{
    static const jam::Function::Map<juce::String, juce::StringPairArray> shellEnvironments { [] {
        jam::Function::Map<juce::String, juce::StringPairArray> table;

        table.add<const juce::File&> ("zsh",
            [] (const juce::File& directory)
            {
                juce::StringPairArray environment;
                const auto parentZdotdir { juce::SystemStats::getEnvironmentVariable ("ZDOTDIR", {}) };

                if (parentZdotdir.isNotEmpty())
                    environment.set ("EVE_ORIG_ZDOTDIR", parentZdotdir);

                environment.set ("ZDOTDIR", directory.getChildFile (juce::String { zshDirectory.data() }).getFullPathName());
                return environment;
            });
        table.add<const juce::File&> ("bash",
            [] (const juce::File& directory)
            {
                juce::StringPairArray environment;

                environment.set ("ENV", directory.getChildFile (juce::String { bashIntegrationFile.data() }).getFullPathName());
                environment.set ("EVE_BASH_INJECT", "1");
                environment.set ("EVE_BASH_UNEXPORT_HISTFILE", "1");
                return environment;
            });
        return table;
    }() };

    const auto key { juce::File (shell).getFileNameWithoutExtension() };

    auto environment { shellEnvironments.contains (key) ? shellEnvironments.get (key, directory) : juce::StringPairArray {} };

    environment.set ("CHERE_INVOKING", "1");
    return environment;
}

juce::String EVETerminalProcessor::getShellArguments (const juce::String& shell, const juce::File& directory)
{
    static const jam::Function::Map<juce::String, juce::String> shellArguments { [] {
        jam::Function::Map<juce::String, juce::String> table;
        const auto getPowershellArguments { [] (const juce::File& directory)
                                         {
                                             return "-NoLogo -NoProfile -NoExit -Command \". '" + directory.getChildFile (juce::String { powershellIntegrationFile.data() }).getFullPathName() + "'\"";
                                         } };

        table.add<const juce::File&> ("bash", [] (const juce::File&) { return juce::String { "--posix" }; });
        table.add<const juce::File&> ("powershell", getPowershellArguments);
        table.add<const juce::File&> ("pwsh", getPowershellArguments);
        return table;
    }() };

    const auto key { juce::File (shell).getFileNameWithoutExtension() };

    if (shellArguments.contains (key)) return shellArguments.get (key, directory);

    return {};
}

jam::TerminalWinsize EVETerminalProcessor::getWinsize() const
{
    return jam::TerminalWinsize::unpack (static_cast<juce::int64> (model.getOrCreateComponentProperty (Id::toTag (Id::winsize), Id::value).getValue()));
}

void EVETerminalProcessor::prepare (const juce::dsp::ProcessSpec& spec)
{
    jassert (spec.maximumBlockSize > 0);

    input.prepare (static_cast<int> (spec.maximumBlockSize));
    terminalProcessor.prepare (static_cast<int> (spec.maximumBlockSize));

    if (const auto winsize { getWinsize() }; winsize.isValid())
        prepare (winsize, getHistoryCapacity());
}

void EVETerminalProcessor::prepare (jam::TerminalWinsize winsize, int historyCapacity)
{
    if (processor.getBlockSize() > 0)
    {
        triggerAsyncUpdate();
        handleUpdateNowIfNeeded();

        const auto blocksPerVblank { juce::jmax (1, static_cast<int> (std::ceil (processor.getSampleRate() / processor.getBlockSize() / jam::VulkanEngine::getRefreshRateHz()))) };

        terminalBuffer.prepare (winsize.cols, winsize.rows, blocksPerVblank * processor.getBlockSize());
        swap.prepare (winsize);
        terminalProcessor.reset (terminalBuffer);
        swap.setCapacity (historyCapacity);

        if (tty->isRunning())
        {
            tty->setWinsize (winsize);
        }
        else
        {
            const auto shell { juce::SystemStats::getEnvironmentVariable (juce::String { shellVariable.data() }, juce::String { defaultShell.data() }) };
            const auto directory { jam::ParameterManager::getInstance()->getUserSettings().getParentDirectory().getChildFile (files::shellIntegrationDirectory) };
            const auto isIntegrated { isShellIntegrated() };

            if (isIntegrated)
                addShellScripts (directory);

            [[maybe_unused]] const auto isOpen { tty->open (processor.getBlockSize(), winsize.toCellRect(), shell, isIntegrated ? getShellArguments (shell, directory) : juce::String {}, getStartDirectory().getFullPathName(), isIntegrated ? getShellEnvironment (shell, directory) : juce::StringPairArray {}) };
            jassert (isOpen);
        }
    }
}

void EVETerminalProcessor::process() noexcept
{
    if (not isBypassed.load())
    {
        input.writePendingData (*tty);

        bool isReading { false };

        do
        {
            isReading = terminalProcessor.process (*tty, terminalBuffer, swap, events);
        }
        while (isReading);
    }
}

void EVETerminalProcessor::setWinsize()
{
    processor.suspendProcessing (true);

    const auto winsize { getWinsize() };
    const auto bounds { terminalBuffer.getBounds() };

    if (winsize.isValid() and (winsize.cols != bounds.width or winsize.rows != bounds.height))
        prepare (winsize, getHistoryCapacity());

    processor.suspendProcessing (false);
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

void EVETerminalProcessor::setHistoryCapacity()
{
    processor.suspendProcessing (true);

    if (const auto winsize { getWinsize() }; winsize.isValid())
        prepare (winsize, getHistoryCapacity());

    processor.suspendProcessing (false);
}
