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
    /**
     * @brief Writes the profiler samples in a profile standalone build. The members then release themselves in reverse order of declaration.
     *
     * In a profile standalone build, the destructor drains the profiler.
     * It then writes the samples file that the manifest names. The manifest is the first command-line argument.
     * In other builds, the destructor does nothing else.
     */
    ~EVEProcessor() override;

    /**
     * @brief Tells the terminal processor to prepare, which starts its clock.
     *
     * The terminal chain does not use the sample rate and the block size.
     *
     * @param sampleRate       Sample rate of the host. The call does not use it.
     * @param samplesPerBlock  Block size of the host. The call does not use it.
     */
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;

    /** @brief Tells the terminal processor to stop its clock. */
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
     * When the data holds a valid tree, the call restores the state inside `terminalProcessor.restart`.
     * The clock is stopped while the terminal Model and the APVTS take the restored tree.
     * The clock then starts again when it ran before.
     *
     * @param data         Bytes that `getStateInformation` wrote.
     * @param sizeInBytes  Number of bytes.
     */
    void setStateInformation (const void* data, int sizeInBytes) override;

    /** @brief Returns the terminal processor of the plugin. */
    EVETerminalProcessor& getTerminalProcessor() noexcept;

    /** @brief Tells the terminal processor to restart its clock. */
    void restart();

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

    jam::PluginEditorLayout layout {
        jam::PluginEditorLayout::create<jam::SettingsDocument<jam::ConfigDocument>> (
            BinaryData::fetcher,
            files::viewLayout,
            files::viewLayout)
    };

    jam::ParameterManager parameterManager {
        files::markdownSettingsDirectory,
        files::markdownSettingsFileName,
        &jam::SettingsDocument<jam::ConfigDocument>::getSettingsValue,
        &jam::SettingsDocument<jam::ConfigDocument>::setSettingsValue
    };

    jam::AudioModel model { parameterManager,
                            *this,
                            jam::ParameterLayout::get (jam::MarkdownDocument::getOrCreate (
                                juce::Identifier { files::parametersLayout })) };

    jam::SharedInstance<jam::StyleManager> styleManager;

#if JAM_USING_PROFILER
    jam::Profiler profiler;
#endif

    EVETerminalProcessor terminalProcessor { *this, model };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EVEProcessor)
};
