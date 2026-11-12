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

#include <JuceHeader.h>
#include "Identifiers.h"

namespace Id
{
/*_____________________________________________________________________________*/

/** @brief Product identifier and name-string vocabulary — branding, buttons, view component types, and images — not already carried by jam. */

    const juce::Identifier args                 { juce::String::fromUTF8 ("args") };
    const juce::Identifier button               { juce::String::fromUTF8 ("button") };
    const juce::Identifier numCommands          { juce::String::fromUTF8 ("numCommands") };
    const juce::Identifier numGridEvents        { juce::String::fromUTF8 ("numGridEvents") };
    const juce::Identifier deviceClock          { juce::String::fromUTF8 ("deviceClock") };
    const juce::Identifier historyCapacity      { juce::String::fromUTF8 ("historyCapacity") };
    const juce::Identifier openWith             { juce::String::fromUTF8 ("openWith") };
    const juce::Identifier pluginState          { juce::String::fromUTF8 ("pluginState") };
    const juce::Identifier previewComponent     { juce::String::fromUTF8 ("previewComponent") };
    const juce::Identifier profiler             { juce::String::fromUTF8 ("profiler") };
    const juce::Identifier selector             { juce::String::fromUTF8 ("selector") };
    const juce::Identifier shell                { juce::String::fromUTF8 ("shell") };
    const juce::Identifier shellIntegration     { juce::String::fromUTF8 ("shellIntegration") };
    const juce::Identifier shellNotFound        { juce::String::fromUTF8 ("shellNotFound") };
    const juce::Identifier terminalInput        { juce::String::fromUTF8 ("terminalInput") };
    const juce::Identifier textEditor           { juce::String::fromUTF8 ("textEditor") };
    const juce::Identifier startDirectory       { juce::String::fromUTF8 ("startDirectory") };
    const juce::Identifier zoom                 { juce::String::fromUTF8 ("zoom") };
    const juce::Identifier zoomStep             { juce::String::fromUTF8 ("zoomStep") };
    const juce::String     failNoOpenFile       { juce::String::fromUTF8 ("open with: no file ") };
    const juce::String     failShellExited      { juce::String::fromUTF8 ("shell exited") };
    const juce::String     failShellNotFound    { juce::String::fromUTF8 ("shell not found: ") };
    const juce::String     shellVariable        { juce::String::fromUTF8 ("SHELL") };
    const juce::String     defaultShell         { juce::String::fromUTF8 ("/bin/zsh") };
    const juce::String     defaultShellWindows  { juce::String::fromUTF8 ("C:/msys64/usr/bin/zsh.exe -l") };
    const juce::String     powershellProgram    { juce::String::fromUTF8 ("powershell.exe") };
    const juce::String     commandPromptProgram { juce::String::fromUTF8 ("cmd.exe") };

/**______________________________END OF NAMESPACE______________________________*/
}// namespace Id
