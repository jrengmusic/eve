#include "EVETerminalProcessor.h"

static constexpr std::string_view shellVariable { "SHELL" };
#if JUCE_WINDOWS
static constexpr std::string_view defaultShell { "powershell.exe" };
#else
static constexpr std::string_view defaultShell { "/bin/zsh" };
#endif

EVETerminalProcessor::EVETerminalProcessor (juce::AudioProcessor& processorToUse, jam::AudioModel& modelToUse)
    : processor (processorToUse), model (modelToUse)
{
    input.prepare (static_cast<int> (jam::TerminalTTY::readChunkSize));
    registerParameterSetters();
    registerEvents();
    registerUserInterfaceGetters();
    registerChainEvents();
    model.addListener (*this);
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
    for (const auto& id : { Id::applicationCursor, Id::bracketedPaste, Id::mouseSgr, Id::cursorVisible, Id::alternateScreen })
        events.add<bool> (id, [this, parameterID = Id::toTag (id)] (bool value) { setParameterValue (parameterID, static_cast<float> (value)); });

    for (const auto& id : { Id::mouseTracking, Id::cursorLine, Id::cursorColumn })
        events.add<int> (id, [this, parameterID = Id::toTag (id)] (int value) { setParameterValue (parameterID, static_cast<float> (value)); });
}

void EVETerminalProcessor::registerUserInterfaceGetters()
{
    userInterfaceGetters[Id::textEditor].add<const jam::TerminalProcessor::Visit&> (
        Id::toType (Id::winsize),
        [this] (const jam::TerminalProcessor::Visit& visit)
        {
            const auto isLiveReplaced { terminalProcessor.pull() };
            visit (terminalProcessor.getHistoryDocument(), terminalProcessor.getLiveDocument(), isLiveReplaced);
        });
}

void EVETerminalProcessor::registerChainEvents()
{
    chainEvents[Id::terminalInput].add<const char*, int> ([this] (const char* bytes, int size)
                                                          {
                                                              if (input.getFreeSpace() >= size)
                                                              {
                                                                  input.write (bytes, static_cast<size_t> (size));
                                                                  input.flush();
                                                              }
                                                          });
}

void EVETerminalProcessor::setParameterValue (juce::StringRef parameterID, float value)
{
    auto* parameter { model.getParameter (parameterID) };
    jassert (parameter != nullptr);

    const auto normalisedValue { parameter->convertTo0to1 (value) };

    if (not juce::exactlyEqual (parameter->getValue(), normalisedValue))
        parameter->setValueNotifyingHost (normalisedValue);
}

void EVETerminalProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (jam::Map::contains (parameters, parameterID))
        parameters.get (parameterID, static_cast<double> (newValue));
}

int EVETerminalProcessor::getHistoryCapacity() const
{
    return static_cast<int> (jam::Model::getChildWithID (model.state, Id::toTag (Id::historyCapacity)).getProperty (Id::value));
}

jam::TerminalWinsize EVETerminalProcessor::getWinsize() const
{
    return jam::TerminalWinsize::unpack (static_cast<juce::int64> (model.getValue (Id::toTag (Id::winsize), Id::value).getValue()));
}

void EVETerminalProcessor::prepare (const juce::dsp::ProcessSpec&)
{
    const auto winsize { getWinsize() };
    const auto bounds { engine.getBounds() };

    if (winsize.isValid() and (winsize.cols != bounds.width or winsize.rows != bounds.height))
    {
        engine.prepare (winsize.cols, winsize.rows);
        terminalProcessor.prepare (getHistoryCapacity(), engine);

        if (tty->isRunning())
        {
            tty->setWinsize (winsize);
        }
        else
        {
            [[maybe_unused]] const auto isOpen { tty->open (winsize.toCellRect(), juce::SystemStats::getEnvironmentVariable (juce::String { shellVariable.data() }, juce::String { defaultShell.data() })) };
            jassert (isOpen);
        }
    }
}

void EVETerminalProcessor::process() noexcept
{
    if (not isBypassed.load())
    {
        input.pull (*tty);
        terminalProcessor.process (*tty, engine, events);
    }
}

void EVETerminalProcessor::setWinsize()
{
    processor.suspendProcessing (true);
    prepare ({});
    processor.suspendProcessing (false);
}
