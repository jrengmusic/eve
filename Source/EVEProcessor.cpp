#include "EVEView.h"
#include "EVEProcessor.h"

static constexpr double virtualSampleRate { 48000.0 };
static constexpr int virtualBlockSize { 4 };

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
    layout.populateTree (model, layout.getOrCreateSettings());
    terminalProcessor.setWinsize();

#if JucePlugin_Build_Standalone
    if (wrapperType == juce::AudioProcessor::wrapperType_Standalone)
    {
        terminalProcessor.onProcessRequest = [this] { device.requestProcess(); };

        juce::MessageManager::callAsync ([this]
                                         {
                                             auto* holder { juce::StandalonePluginHolder::getInstance() };
                                             jassert (holder != nullptr);

                                             holder->deviceManager.closeAudioDevice();
                                             device.open ({}, {}, virtualSampleRate, virtualBlockSize);
                                             device.start (&holder->player);
                                             device.requestProcess();
                                         });
    }
#endif
}

EVEProcessor::~EVEProcessor()
{
    device.stop();
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

int EVEProcessor::getNumPrograms()
{
    return 1;
}

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
    suspendProcessing (true);

    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32> (getTotalNumInputChannels());

    terminalProcessor.prepare (spec);

    suspendProcessing (false);
}

void EVEProcessor::releaseResources() {}

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

template<typename SampleType>
void EVEProcessor::process (juce::AudioBuffer<SampleType>& buffer)
{
    juce::ScopedNoDenormals noDenormals;
    auto nInput { getTotalNumInputChannels() };
    auto nOutput { getTotalNumOutputChannels() };

    for (auto i { nInput }; i < nOutput; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    terminalProcessor.process();
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

bool EVEProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* EVEProcessor::createEditor()
{
    return new EVEView (model, layout, *this);
}

void EVEProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream { destData, false };
    model.copyState().writeToStream (stream);
}

void EVEProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto state { juce::ValueTree::readFromData (data, sizeInBytes) }; state.isValid())
    {
        model.setState (state);
        terminalProcessor.setWinsize();
    }
}

EVETerminalProcessor& EVEProcessor::getTerminalProcessor() noexcept
{
    return terminalProcessor;
}

void EVEProcessor::setWinsize()
{
    terminalProcessor.setWinsize();
}

juce::AudioProcessorParameter* EVEProcessor::getBypassParameter() const
{
    return model.getParameter (Id::toTag (Id::bypass));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new EVEProcessor(); }
