#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"

/**
 * @class EVETerminalProcessor
 * @brief Terminal chain of the plugin. It presents the rows of the engine ring to the `jam::TerminalProcessor`, which owns the live document and the history record.
 *
 * `process()` makes the engine write the
 * rows to the history FIFO and the live FIFO of the terminal processor. The terminal processor owns the history `jam::SwapDocument`.
 * The view pulls through the getter in `userInterfaceGetters`: the history bytes append to the history
 * record, and the view reads the record and the live document inside the call and stores neither.
 *
 * @par Thread contract
 * `process()` runs on the audio thread. It writes the input lane to the TTY, then decodes the TTY output.
 * `setWinsize()` runs on the message thread. When the cell grid of the model winsize differs from the grid of the engine,
 * it calls `prepare()` between `suspendProcessing (true)` and `suspendProcessing (false)`, so `process()` does not run during the change.
 * `prepare()` runs on the message thread.
 */
class EVETerminalProcessor : public juce::AudioProcessorValueTreeState::Listener
{
public:
    /**
     * @brief Registers the parameter setters, the events, the user interface getters and the chain events, then listens to the model.
     * @param processorToUse  Processor that owns this object. The object suspends it while `setWinsize()` prepares.
     * @param modelToUse      Model that holds the history capacity setting. It must outlive this object.
     *                        The setting sizes the history FIFO.
     */
    EVETerminalProcessor (juce::AudioProcessor& processorToUse, jam::AudioModel& modelToUse);

    /**
     * @brief Prepares the engine, sizes both FIFOs of the terminal processor and sizes or opens the TTY.
     *
     * The function does nothing unless the model winsize is valid and its cell grid differs from the grid of the engine.
     * The terminal processor first drains the history FIFO into the history record.
     * The TTY takes the new size when it runs. Otherwise the function opens it with the shell of the `SHELL` variable.
     * The live FIFO holds the bytes of one frame of the current window size. The history FIFO holds
     * the live bytes, the bytes of the rows that the engine holds now, and the bytes of a grid as wide as the
     * current window and as tall as the history capacity of the model (jassert > 0).
     * The history capacity sizes only the history FIFO. It does not limit the history record.
     */
    void prepare (const juce::dsp::ProcessSpec&);

    /** @brief Audio thread. Unless bypassed, it writes the input lane to the TTY, then decodes the TTY output and makes the engine write its ready rows to the FIFOs. The live area has the row count of the current window size. */
    void process() noexcept;

    /** @brief Prepares again from the model winsize, under `suspendProcessing`. `prepare()` changes the engine only when the cell grid differs. */
    void setWinsize();

    /**
     * @brief Stores the bypass state when the bypass parameter changes.
     * @param parameterID  ID of the parameter that changed; only the bypass parameter has a setter.
     * @param newValue     New parameter value; a non-zero value bypasses.
     */
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    jam::ViewManager::Getters userInterfaceGetters;     ///< Getters for the view. The constructor adds the text editor getter. It pulls the terminal processor, then gives the history record, the live document, and whether the pull replaced the live document to a `jam::TerminalProcessor::Visit`.
    jam::ViewManager::Events chainEvents;               ///< Events of the chain. The constructor adds the `terminalInput` event. It writes its bytes to the input lane when the lane has room for them.

private:
    /** @brief Adds the bypass setter to the parameter setters. */
    void registerParameterSetters();

    /** @brief Adds the event setters that copy the terminal state to the model parameters. */
    void registerEvents();

    /** @brief Adds the text editor getter keyed by the winsize type. It pulls the terminal processor, then calls the visitor with the history record, the live document, and whether the pull replaced the live document. */
    void registerUserInterfaceGetters();

    /** @brief Adds the `terminalInput` event to the chain events. It writes bytes to the input lane and flushes the lane when the lane has room for them. */
    void registerChainEvents();

    /**
     * @brief Sets a model parameter to a value and tells the host, only when the value differs.
     * @param parameterID  ID of the parameter.
     * @param value        Value in the range of the parameter.
     */
    void setParameterValue (juce::StringRef parameterID, float value);

    /** @brief Returns the history capacity setting from the current state tree. `prepare()` uses it to size the history FIFO. */
    int getHistoryCapacity() const;

    /** @brief Returns the winsize that the model holds. */
    jam::TerminalWinsize getWinsize() const;

    /** @brief Returns the TTY of the platform. */
    static std::unique_ptr<jam::TerminalTTY> createTTY();

    juce::AudioProcessor& processor;                  ///< Processor that `setWinsize()` suspends.
    jam::AudioModel& model;                           ///< Model that holds the history capacity setting.
    std::atomic<bool> isBypassed { false };           ///< Bypass state. `parameterChanged()` writes it, and `process()` reads it.
    jam::Function::Map<juce::String, void> parameters;   ///< Parameter setters, keyed by parameter ID.
    jam::Function::Map<juce::Identifier, void> events;   ///< Event setters that copy the terminal state to the model parameters, keyed by event ID.
    jam::TerminalFIFO input;                          ///< Input lane from the view to the TTY.
    std::unique_ptr<jam::TerminalTTY> tty { createTTY() };   ///< Pseudo terminal of the shell.
    jam::TerminalProcessor terminalProcessor;           ///< History FIFO, live FIFO, the live document and the history record.

    static constexpr bool shouldUseInlineImages { true };   ///< Whether the engine presents image placements as inline images.

    jam::TerminalGraphicsEngine engine { shouldUseInlineImages };     ///< Row grid that `process()` presents.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EVETerminalProcessor)
};
