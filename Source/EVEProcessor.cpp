#include "EVEProcessor.h"
#include "ENDView.h"

EVEProcessor::EVEProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    , model (parameterManager, *this, {})
{
}

void EVEProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (sampleRate, samplesPerBlock);
}

void EVEProcessor::releaseResources() {}

bool EVEProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet()
           and (layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
                or layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
}

void EVEProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    for (auto channel { getTotalNumInputChannels() }; channel < getTotalNumOutputChannels();
         ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* EVEProcessor::createEditor()
{
    return new ENDView (model, layout, *this);
}

bool EVEProcessor::hasEditor() const { return true; }

const juce::String EVEProcessor::getName() const { return JucePlugin_Name; }

bool EVEProcessor::acceptsMidi() const { return false; }
bool EVEProcessor::producesMidi() const { return false; }
bool EVEProcessor::isMidiEffect() const { return false; }
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

void EVEProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml { model.copyState().createXml() })
        copyXmlToBinary (*xml, destData);
}

void EVEProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml { getXmlFromBinary (data, sizeInBytes) })
        model.setState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new EVEProcessor(); }
