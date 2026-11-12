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
extern const juce::Identifier numCommands;
extern const juce::Identifier numGridEvents;
extern const juce::Identifier deviceClock;
extern const juce::Identifier historyCapacity;
extern const juce::Identifier openWith;
extern const juce::Identifier pluginState;
extern const juce::Identifier previewComponent;
extern const juce::Identifier profiler;
extern const juce::Identifier selector;
extern const juce::Identifier shell;
extern const juce::Identifier shellIntegration;
extern const juce::Identifier shellNotFound;
extern const juce::Identifier terminalInput;
extern const juce::Identifier textEditor;
extern const juce::Identifier startDirectory;
extern const juce::Identifier zoom;
extern const juce::Identifier zoomStep;
extern const juce::String     failNoOpenFile;
extern const juce::String     failShellExited;
extern const juce::String     failShellNotFound;
extern const juce::String     shellVariable;
extern const juce::String     defaultShell;
extern const juce::String     defaultShellWindows;
extern const juce::String     powershellProgram;
extern const juce::String     commandPromptProgram;

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
            { openFile,  juce::Identifier { "openFile" } },
            { markdown,  juce::Identifier { "markdown" } },
            { dismiss,   juce::Identifier { "dismiss" } },
            { zoomIn,    juce::Identifier { "zoomIn" } },
            { zoomOut,   juce::Identifier { "zoomOut" } },
            { zoomReset, juce::Identifier { "zoomReset" } },
            { settings,  juce::Identifier { "settings" } },
    } } {}

    enum value : int
    {
        openFile  = 0x4600028,
        markdown  = 0x4600029,
        dismiss   = 0x460002a,
        zoomIn    = 0x460002b,
        zoomOut   = 0x460002c,
        zoomReset = 0x460002d,
        settings  = 0x460002e,
    };

    static CommandId* getInstance() noexcept
    {
        return jam::SharedInstance<CommandId>::getInstance();
    }
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace map
