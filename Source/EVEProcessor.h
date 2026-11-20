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
     * @brief Removes the processor and the connection from the listeners of the model state.
     *
     * In a profile build, the call first opens the `processorDelete` zone. The zone ends when its member is destroyed,
     * after this body. The members release in reverse order of declaration.
     */
    ~EVEProcessor() override;

    /**
     * @brief Tells the terminal processor to prepare. It stops its clock, then opens the TTY when none exists and the window size is valid.
     *
     * The clock starts only when the window size is valid and a TTY exists after that open attempt. Without a valid window size, the clock stays stopped.
     * A failed TTY open leaves no TTY, so the clock stays stopped.
     * The terminal chain does not use the sample rate and the block size.
     *
     * @param sampleRate       Sample rate of the host. The call does not use it.
     * @param samplesPerBlock  Block size of the host. The call does not use it.
     */
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;

    /**
     * @brief Tells the terminal processor to release its resources. It stops the clock, cancels the pending update, then closes the clock.
     *
     * The TTY stays open. releaseResources() does not close it.
     */
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
     * @brief Writes the model state to @p destData, so a host can restore it.
     *
     * The call flushes the terminal Model to its tree first, so the `terminal` child holds the current state.
     * It then removes the model rows that `parameters.md` marks runtime, and the `gridStarts` child, from the state that it writes.
     *
     * @param destData  Block that receives the state bytes.
     */
    void getStateInformation (juce::MemoryBlock& destData) override;

    /**
     * @brief Restores the model state from host data.
     *
     * Bytes that do not form a valid tree leave the state unchanged.
     * A valid tree suspends the audio processing. The terminal chain then takes the values of the `terminal` child through
     * `EVETerminalProcessor::setState()`, and the model takes the tree. The audio processing then resumes.
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
     * @brief Passes a changed model row to the host display or to the terminal chain.
     *
     * The row `winsize` calls `terminalProcessor.prepare()`.
     * The rows `title`, `foregroundProcess`, `cwd`, `zoom`, `shellExited` and `markdownFile` call `updateHostDisplay()` with the
     * parameter info and the non-parameter state flagged as changed.
     * With `JAM_USING_PROFILER` set, the `numPrompts` row resets `launchPrompt`.
     * Any other row has no effect.
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
            [this] (const auto& fallback) { applyRule (Id::engine, fallback); })
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

    /**
     * @brief Client connection to the server of END. It carries the model state of this plugin.
     *
     * The connection is declared after model, so it is destroyed before model. The constructor adds it as a listener of
     * model.state. setStateInformation() restores through jam::AudioModel::setState(), whose APVTS replaceState() assigns
     * a new root to model.state. That assignment redirects the root, and the connection, as a listener, adopts it.
     * The constructor runs the server rule of the ENAX extensions, which connects the connection. That rule is registered
     * only under END.
     */
    jam::ValueTreeConnection connection { model.state };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EVEProcessor)
};
