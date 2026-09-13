#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"

/**
 * @class EVEAudioProcessor
 * @brief Owns the callback tables the editor binds its component tree
 *        through, keyed and populated by jam::ViewManager::build().
 */
class EVEAudioProcessor
{
public:
    EVEAudioProcessor() = default;

    /** Processor getter callbacks, keyed first by component type, then by parameter ID. */
    jam::ViewManager::Getters userInterfaceGetters;

    /** Callback arrays contributed by bound components, keyed by event name. */
    jam::ViewManager::Events chainEvents;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EVEAudioProcessor)
};
