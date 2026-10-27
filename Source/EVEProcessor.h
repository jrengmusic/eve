#pragma once
#include "EVETerminalProcessor.h"

class EVEProcessor
    : public juce::AudioProcessor
#if JucePlugin_Enable_ARA
    , public juce::AudioProcessorARAExtension
#endif
{
public:
    EVEProcessor();
    /** @brief Defaulted. The members release themselves in reverse order of declaration. */
    ~EVEProcessor() override;

    /** @brief Suspends processing, stores the sample rate and block size in the process spec, prepares the terminal processor on the context of the virtual device, then resumes processing. */
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

    bool supportsDoublePrecisionProcessing() const override;

    template<typename SampleType>
    void process (juce::AudioBuffer<SampleType>& buffer);

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;

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

    /** @brief Flushes the terminal Model to its tree, then writes a copy of the model state to @p destData. */
    void getStateInformation (juce::MemoryBlock& destData) override;

    /**
     * @brief Restores the model state from host data.
     *
     * When the data holds a valid tree, the call suspends processing, joins the terminal Model to the tree,
     * sets the model state, resumes processing, and prepares again.
     *
     * @param data         Bytes that `getStateInformation` wrote.
     * @param sizeInBytes  Number of bytes.
     */
    void setStateInformation (const void* data, int sizeInBytes) override;

    /** @brief Returns the terminal processor of the plugin. */
    EVETerminalProcessor& getTerminalProcessor() noexcept;

    /** @brief Calls `prepareToPlay` with the current sample rate and block size when a block size exists, so the terminal processor prepares again. */
    void prepare();

    /** @brief Tells the terminal processor to clear. */
    void clear();

    /** @brief Returns the bypass parameter of the model, for the host. */
    juce::AudioProcessorParameter* getBypassParameter() const override;

private:
    juce::AudioProcessorEditor* createEditor() override;

#if JUCE_DEBUG
    jam::debug::Log::Scope logScope { jam::File::getDebugLog() };
#endif
    Generated generated;

    jam::PluginEditorLayout layout { jam::PluginEditorLayout::create<jam::SettingsDocument<jam::ConfigDocument>> (BinaryData::fetcher, files::viewLayout, files::viewLayout) };

    jam::ParameterManager parameterManager { files::markdownSettingsDirectory, files::markdownSettingsFileName, &jam::SettingsDocument<jam::ConfigDocument>::getSettingsValue, &jam::SettingsDocument<jam::ConfigDocument>::setSettingsValue };

    jam::AudioModel model { parameterManager, *this, jam::ParameterLayout::get (jam::MarkdownDocument::getOrCreate (juce::Identifier { files::parametersLayout })) };

    jam::VirtualDevice device { jam::VirtualDeviceType::virtualDeviceTypeName, jam::VirtualDeviceType::virtualDeviceTypeName };

    EVETerminalProcessor terminalProcessor { *this, model };

    juce::dsp::ProcessSpec spec;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EVEProcessor)
};
