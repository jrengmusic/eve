#pragma once
#include "EVETerminalProcessor.h"

class EVEProcessor
    : public juce::AudioProcessor
#if JucePlugin_Enable_ARA
    , public juce::AudioProcessorARAExtension
#endif
    , public jam::ENAXClientExtensions
    , private juce::ValueTree::Listener
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

    /**
     * @brief Writes the model state to @p destData, with the terminal tree saved as the pluginState child.
     *
     * The call flushes the terminal Model to its tree first.
     * The `terminal` child is copied to a `pluginState` child, which replaces it in the written state.
     * A host holds the `pluginState` child as the state of this plugin instance.
     *
     * @param destData  Block that receives the state bytes.
     */
    void getStateInformation (juce::MemoryBlock& destData) override;

    /**
     * @brief Restores the model state from host data.
     *
     * A tree of the `pluginState` type sets each of its properties on the terminal Model through `EVETerminalProcessor::setRows()`.
     * It does not restart the clock.
     * Any other valid tree restores the state inside `terminalProcessor.restart`.
     * The clock is stopped while the terminal Model and the APVTS take the restored tree.
     * The clock then starts again when it ran before.
     *
     * @param data         Bytes that `getStateInformation` wrote.
     * @param sizeInBytes  Number of bytes.
     */
    void setStateInformation (const void* data, int sizeInBytes) override;

    /** @brief Returns the terminal processor of the plugin. */
    EVETerminalProcessor& getTerminalProcessor() noexcept;

    /** @brief Returns the bypass parameter of the model, for the host. */
    juce::AudioProcessorParameter* getBypassParameter() const override;

private:
    juce::AudioProcessorEditor* createEditor() override;

    /**
     * @brief Tells the host that the display changed, when a host-visible terminal row changed.
     *
     * The rows `title`, `foregroundProcess`, `cwd`, `zoom` and `shellExited` call `updateHostDisplay()` with the
     * parameter info and the non-parameter state flagged as changed. Any other row has no effect.
     *
     * @param tree      Tree that changed. Not used.
     * @param property  Property that changed.
     */
    void valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property) override;

    jam::debug::Log::Scope logScope { jam::File::getDebugLog() };
    Generated generated;

#if JAM_USING_PROFILER
    jam::SharedInstance<jam::Profiler> profiler { std::in_place, jam::Profiler::defaultCapacity };
    std::optional<jam::Profiler::Zone> processorDelete;
    std::optional<jam::Profiler::Zone> launchPrompt { std::in_place, Id::launchPrompt };
    std::optional<jam::Profiler::Zone> processorLayout { std::in_place, Id::processorLayout };
#endif

    jam::PluginEditorLayout layout {
        jam::PluginEditorLayout::create<jam::SettingsDocument<jam::ConfigDocument>> (
            BinaryData::fetcher,
            files::viewLayout,
            files::viewLayout,
            [this] (const auto& fallback) { set (Id::engine, fallback); })
    };

    jam::ParameterManager parameterManager {
        files::markdownSettingsDirectory,
        files::markdownSettingsFileName,
        &jam::SettingsDocument<jam::ConfigDocument>::getSettingsValue,
        &jam::SettingsDocument<jam::ConfigDocument>::setSettingsValue
    };

    jam::AudioModel model { parameterManager,
                            *this,
                            jam::ParameterLayout::get (*jam::MarkdownDocument::getOrCreate (juce::Identifier { files::parametersLayout })) };

    jam::SharedInstance<jam::StyleManager> styleManager;

    EVETerminalProcessor terminalProcessor { *this, model };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EVEProcessor)
};
