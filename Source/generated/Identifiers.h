/*******************************************************************************
                        Codegen Annotated Source of Truth
————————————————————————————————————————————————————————————————————————————————

            ░░████████████░░████████████░░████████████░░████████████
            ░░████  ░░████░░████  ░░████░░████  ░░████    ░░████
            ░░████        ░░████  ░░████░░████            ░░████
            ░░████        ░░████████████░░████████████    ░░████
            ░░████        ░░████  ░░████        ░░████    ░░████
            ░░████  ░░████░░████  ░░████░░████  ░░████    ░░████
            ░░████████████░░████  ░░████░░████████████    ░░████

————————————————————————————————————————————————————————————————————————————————
                         FOR YOUR EYES ONLY, DO NOT EDIT
********************************************************************************/

/**
 * @file Identifiers.h
 * @brief Product identifier vocabulary.
 */

#pragma once

namespace Id
{
/*_____________________________________________________________________________*/

/** @brief Product identifier and name-string vocabulary — branding, buttons, view component types, and images — not already carried by jam. */

extern const juce::Identifier args;
extern const juce::Identifier button;
extern const juce::Identifier colorMode;
extern const juce::Identifier commandCount;
extern const juce::Identifier deviceClock;
extern const juce::Identifier historyCapacity;
extern const juce::Identifier previewComponent;
extern const juce::Identifier previewWidth;
extern const juce::Identifier profiler;
extern const juce::Identifier selector;
extern const juce::Identifier shell;
extern const juce::Identifier shellIntegration;
extern const juce::Identifier terminalInput;
extern const juce::Identifier textEditor;

/**______________________________END OF NAMESPACE______________________________*/
}// namespace Id
namespace map
{
/*_____________________________________________________________________________*/

/**
 * @brief EVE command-id to command-name map.
 *
 * Maps each command-id ordinal to the command name a config `keys` row uses.
 * The key is the juce::CommandID; the value is the row key.
 */
struct CommandId : public jam::Bimap<int, juce::Identifier>
{
    CommandId() : jam::Bimap<int, juce::Identifier> { {
            { clear, juce::Identifier { "clear" } },
    } } {}

    enum value : int
    {
        clear = 0x4600012,
    };

    static CommandId* getInstance() noexcept
    {
        return jam::SharedInstance<CommandId>::getInstance();
    }
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace map
