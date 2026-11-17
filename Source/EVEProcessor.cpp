#include "EVEView.h"
#include "EVEProcessor.h"

EVEProcessor::EVEProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor (BusesProperties()
#if not JucePlugin_IsMidiEffect
#if not JucePlugin_IsSynth
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
#endif
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif// not JucePlugin_IsMidiEffect
      )
#endif// !JucePlugin_PreferredChannelConfigurations
{
    const auto settings { layout.getOrCreateSettings() };

    layout.populateTree (model, settings);

    {
#if JAM_USING_PROFILER
        const jam::Profiler::Zone zone { Id::styleRegisterStyle };
#endif
        styleManager.create (layout.fonts, settings);
    }

    const auto [width, height] { jam::ViewManager::getUISize<jam::MarkdownDocument> (juce::Identifier { files::viewLayout }) };
    auto terminal { model.state.getChildWithName (jam::Format::toType (Id::terminal)) };

    jassert (terminal.isValid());
    terminal.setProperty (Id::width, width, nullptr);
    terminal.setProperty (Id::height, height, nullptr);

    model.state.addListener (this);
#if JAM_USING_PROFILER
    processorLayout.reset();
#endif
}

EVEProcessor::~EVEProcessor()
{
#if JAM_USING_PROFILER
    processorDelete.emplace (Id::processorDelete);
#endif
    model.state.removeListener (this);
}

const juce::String EVEProcessor::getName() const { return JucePlugin_Name; }

bool EVEProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool EVEProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool EVEProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double EVEProcessor::getTailLengthSeconds() const { return 0.0; }

int EVEProcessor::getNumPrograms() { return 1; }

int EVEProcessor::getCurrentProgram() { return 0; }

void EVEProcessor::setCurrentProgram (int index) { juce::ignoreUnused (index); }

const juce::String EVEProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void EVEProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void EVEProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
#if JAM_USING_PROFILER
    const jam::Profiler::Zone zone { Id::processorPrepare };
#endif
    juce::ignoreUnused (sampleRate, samplesPerBlock);
    terminalProcessor.prepare();
}

void EVEProcessor::releaseResources()
{
#if JAM_USING_PROFILER
    const jam::Profiler::Zone zone { Id::processorRelease };
#endif
    terminalProcessor.releaseResources();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool EVEProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        and layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

#if not JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

bool EVEProcessor::supportsDoublePrecisionProcessing() const { return true; }

template <typename SampleType>
void EVEProcessor::process (juce::AudioBuffer<SampleType>& buffer)
{
    juce::ScopedNoDenormals noDenormals;
    auto nInput { getTotalNumInputChannels() };
    auto nOutput { getTotalNumOutputChannels() };

    for (auto i { nInput }; i < nOutput; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
}

void EVEProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    process (buffer);
}

void EVEProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    process (buffer);
}

bool EVEProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* EVEProcessor::createEditor()
{
#if JAM_USING_PROFILER
    const jam::Profiler::Zone zone { Id::processorEditor };
#endif
    return new EVEView (model, layout, *this);
}

void EVEProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    terminalProcessor.flush();

    const auto& document { *jam::MarkdownDocument::getOrCreate (juce::Identifier { files::parametersLayout }) };
    auto state { model.copyState() };
    auto terminal { state.getChildWithName (jam::Format::toType (Id::terminal)) };

    jassert (terminal.isValid());

    for (auto* row : document.getTableRows (Id::model))
        if (document.hasTableValue (*row, Id::runtime))
            terminal.removeProperty (row->id, nullptr);

    terminal.removeChild (terminal.getChildWithName (jam::Format::toType (Id::gridStarts)), nullptr);

    juce::MemoryOutputStream stream { destData, false };
    state.writeToStream (stream);
}

void EVEProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto state { juce::ValueTree::readFromData (data, sizeInBytes) }; state.isValid())
    {
        suspendProcessing (true);
        terminalProcessor.setState (state);
        model.setState (state);
        suspendProcessing (false);
    }
}

void EVEProcessor::valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier& property)
{
#if JAM_USING_PROFILER
    if (property == Id::numPrompts)
        launchPrompt.reset();
#endif

    if (property == Id::winsize)
        terminalProcessor.prepare();

    static const std::array hostRows { Id::title, Id::foregroundProcess, Id::cwd, Id::zoom, Id::shellExited, Id::markdownFile };

    if (std::find (hostRows.begin(), hostRows.end(), property) != hostRows.end())
        updateHostDisplay (ChangeDetails {}.withParameterInfoChanged (true).withNonParameterStateChanged (true));
}

EVETerminalProcessor& EVEProcessor::getTerminalProcessor() noexcept { return terminalProcessor; }

juce::AudioProcessorParameter* EVEProcessor::getBypassParameter() const
{
    return model.getParameter (jam::Format::toTag (Id::bypass));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new EVEProcessor(); }
