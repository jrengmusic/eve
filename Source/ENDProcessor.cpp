#include "ENDProcessor.h"
#include "ENDView.h"

ENDProcessor::ENDProcessor()
    : AudioProcessor (BusesProperties()
                           .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                           .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    , model (parameterManager, *this, {})
{
}

void ENDProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (sampleRate, samplesPerBlock);
}

void ENDProcessor::releaseResources()
{
}

bool ENDProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet()
           and (layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
                or layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
}

void ENDProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    for (auto channel { getTotalNumInputChannels() }; channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* ENDProcessor::createEditor()
{
    return new ENDView (model, layout, *this);
}

bool ENDProcessor::hasEditor() const { return true; }

const juce::String ENDProcessor::getName() const { return JucePlugin_Name; }

bool ENDProcessor::acceptsMidi() const { return false; }
bool ENDProcessor::producesMidi() const { return false; }
bool ENDProcessor::isMidiEffect() const { return false; }
double ENDProcessor::getTailLengthSeconds() const { return 0.0; }

int ENDProcessor::getNumPrograms() { return 1; }
int ENDProcessor::getCurrentProgram() { return 0; }
void ENDProcessor::setCurrentProgram (int index) { juce::ignoreUnused (index); }
const juce::String ENDProcessor::getProgramName (int index) { juce::ignoreUnused (index); return {}; }
void ENDProcessor::changeProgramName (int index, const juce::String& newName) { juce::ignoreUnused (index, newName); }

void ENDProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml { model.copyState().createXml() })
        copyXmlToBinary (*xml, destData);
}

void ENDProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml { getXmlFromBinary (data, sizeInBytes) })
        model.setState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ENDProcessor();
}
