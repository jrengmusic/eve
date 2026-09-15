#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"
#include "EVEAudioProcessor.h"

class EVEProcessor : public juce::AudioProcessor
{
public:
    EVEProcessor();
    ~EVEProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    map::Generated generated;
    jam::PluginEditorLayout layout { BinaryData::fetcher, files::viewLayout, files::defaultConfig };
    jam::ParameterManager parameterManager;
    jam::AudioModel model;
    EVEAudioProcessor audioProcessor;
    jam::TerminalModel terminalModel;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EVEProcessor)
};
