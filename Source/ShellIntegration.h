#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"

/**
 * @class ShellIntegration
 * @brief Shell integration setup of the terminal. It reads the settings, writes the integration scripts, and gives the shell arguments and environment.
 *
 * The class has static members only.
 * The integration scripts exist for zsh, bash and PowerShell.
 */
class ShellIntegration
{
public:
    /**
     * @brief Returns the folder where the shell starts.
     * @return Folder from the `startDirectory` setting. The default is `~`.
     */
    static juce::File getStartDirectory();

    /**
     * @brief Writes the integration scripts into a folder.
     *
     * When the `shellIntegration` setting is off, the call does nothing.
     * Otherwise the call creates the folder and its `zsh` subfolder when they are absent.
     * It writes the zsh environment file and the zsh integration file into the `zsh` subfolder.
     * It writes the bash script, the PowerShell script and the shell command file into the folder, and it sets the execute permission of the shell command file. It replaces files that exist.
     *
     * @param directory  Folder that receives the scripts.
     */
    static void addShellScripts (const juce::File& directory);

    /**
     * @brief Returns the environment variables that the shell receives.
     *
     * For zsh, the variables point `ZDOTDIR` to the `zsh` subfolder of @p directory. When the parent sets `ZDOTDIR`, the call keeps it in `EVE_ORIG_ZDOTDIR`.
     * For bash, the variables point `ENV` to the bash script in @p directory, and they set `EVE_BASH_INJECT` and `EVE_BASH_UNEXPORT_HISTFILE`.
     * When the setting is on, every shell also receives `CHERE_INVOKING`.
     * When the setting is off, the call returns an empty array.
     *
     * @param shell      Shell program name or absolute path. The function keys on the file name without extension.
     * @param directory  Folder that holds the integration scripts.
     * @return           Variables for the shell, in addition to the environment of the parent.
     */
    static juce::StringPairArray getShellEnvironment (const juce::String& shell, const juce::File& directory);

    /**
     * @brief Returns the command-line arguments of the shell.
     *
     * Bash receives `--posix`. PowerShell and pwsh receive the arguments that source the PowerShell script in @p directory.
     * Other shells receive no arguments.
     * When the setting is off, the call returns an empty string for every shell.
     *
     * @param shell      Shell program name or absolute path. The function keys on the file name without extension.
     * @param directory  Folder that holds the integration scripts.
     * @return           Space-separated arguments for the shell. The string is empty when the shell needs none, or when the setting is off.
     */
    static juce::String getShellArguments (const juce::String& shell, const juce::File& directory);

    /**
     * @brief Appends a file path to a command line, quoted for the shell.
     *
     * PowerShell and pwsh double each single quote in the path. Other shells use the POSIX escape for each single quote.
     *
     * @param output  String that receives the quoted path at its end.
     * @param shell   Shell program name or absolute path. The function keys on the file name without extension.
     * @param path    Path to quote.
     */
    static void appendPath (std::string& output, const juce::String& shell, std::string_view path);

    /**
     * @brief Tells whether a path holds a control byte.
     *
     * A byte is a control byte when `std::iscntrl` classifies it as a control character.
     *
     * @param path  Path to test.
     * @return      `true` when any byte of the path is a control byte.
     */
    static bool hasControlByte (std::string_view path);

private:
    /**
     * @brief Tells whether the shell integration setting is on.
     * @return `true` when the `shellIntegration` setting is true. The default is true.
     */
    static bool isShellIntegrated();
};
