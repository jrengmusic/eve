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
extern const juce::Identifier previewWidth;
extern const juce::Identifier profiler;
extern const juce::Identifier selector;
extern const juce::Identifier shell;
extern const juce::Identifier shellIntegration;
extern const juce::Identifier startDirectory;
extern const juce::Identifier terminalInput;
extern const juce::Identifier textEditor;
extern const juce::Identifier winsize;

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
            { zoomIn,          juce::Identifier { "zoomIn" } },
            { zoomOut,         juce::Identifier { "zoomOut" } },
            { zoomReset,       juce::Identifier { "zoomReset" } },
            { settings,        juce::Identifier { "settings" } },
            { left,            juce::Identifier { "left" } },
            { down,            juce::Identifier { "down" } },
            { up,              juce::Identifier { "up" } },
            { right,           juce::Identifier { "right" } },
            { wordForward,     juce::Identifier { "wordForward" } },
            { wordBackward,    juce::Identifier { "wordBackward" } },
            { wordEnd,         juce::Identifier { "wordEnd" } },
            { lineStart,       juce::Identifier { "lineStart" } },
            { lineEnd,         juce::Identifier { "lineEnd" } },
            { documentStart,   juce::Identifier { "documentStart" } },
            { documentEnd,     juce::Identifier { "documentEnd" } },
            { halfPageUp,      juce::Identifier { "halfPageUp" } },
            { halfPageDown,    juce::Identifier { "halfPageDown" } },
            { clear,           juce::Identifier { "clear" } },
            { normal,          juce::Identifier { "normal" } },
            { visualCharacter, juce::Identifier { "visualCharacter" } },
            { visualLine,      juce::Identifier { "visualLine" } },
            { visualBlock,     juce::Identifier { "visualBlock" } },
            { yank,            juce::Identifier { "yank" } },
            { terminal,        juce::Identifier { "terminal" } },
            { preview,         juce::Identifier { "preview" } },
    } } {}

    enum value : int
    {
        zoomIn          = 0x4600001,
        zoomOut         = 0x4600002,
        zoomReset       = 0x4600003,
        settings        = 0x4600004,
        left            = 0x4600005,
        down            = 0x4600006,
        up              = 0x4600007,
        right           = 0x4600008,
        wordForward     = 0x4600009,
        wordBackward    = 0x460000a,
        wordEnd         = 0x460000b,
        lineStart       = 0x460000c,
        lineEnd         = 0x460000d,
        documentStart   = 0x460000e,
        documentEnd     = 0x460000f,
        halfPageUp      = 0x4600010,
        halfPageDown    = 0x4600011,
        clear           = 0x4600012,
        normal          = 0x4600018,
        visualCharacter = 0x4600019,
        visualLine      = 0x460001a,
        visualBlock     = 0x460001b,
        yank            = 0x460001c,
        terminal        = 0x4600026,
        preview         = 0x4600027,
    };

    static CommandId* getInstance() noexcept
    {
        return jam::SharedInstance<CommandId>::getInstance();
    }
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace map
