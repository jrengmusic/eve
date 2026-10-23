#include "EVETerminalProcessor.h"

EVETerminalProcessor::EVETerminalProcessor (jam::AudioModel& modelToUse)
    : model (modelToUse)
{
    registerUserInterfaceGetters();
}

void EVETerminalProcessor::registerUserInterfaceGetters()
{
    userInterfaceGetters[Id::textEditor].add<jam::AnsiDocument&, const std::function<void (jam::AnsiDocument&, bool, int)>&> (
        Id::toType (Id::winsize),
        [this] (jam::AnsiDocument& historyDocument, const std::function<void (jam::AnsiDocument&, bool, int)>& visitLive)
        {
            const auto isLiveReplaced { terminalProcessor.pull (historyDocument) };
            visitLive (terminalProcessor.getLiveDocument(), isLiveReplaced, getHistoryCapacity());
        });
}

int EVETerminalProcessor::getHistoryCapacity() const
{
    return static_cast<int> (jam::Model::getChildWithID (model.state, Id::toTag (Id::historyCapacity)).getProperty (Id::value));
}

void EVETerminalProcessor::prepare (const juce::dsp::ProcessSpec&)
{
    const auto winsize { jam::TerminalWinsize::unpack (pendingWinsize.load()) };

    const auto historyRows { getHistoryCapacity() };
    jassert (historyRows > 0);

    const auto liveCapacity { jam::TerminalGraphicsEngine::getNumBytes (winsize) };

    terminalProcessor.prepare (liveCapacity + engine.getNumBytes() + jam::TerminalGraphicsEngine::getNumBytes (jam::TerminalWinsize { winsize.cols, historyRows }), liveCapacity);
}

void EVETerminalProcessor::reset() noexcept
{
    terminalProcessor.reset();
}

void EVETerminalProcessor::process() noexcept
{
    const auto winsize { jam::TerminalWinsize::unpack (pendingWinsize.load()) };

    terminalProcessor.process (engine, winsize.rows);
}

void EVETerminalProcessor::setWinsize (jam::TerminalWinsize newWinsize)
{
    pendingWinsize.store (newWinsize.pack());
}
