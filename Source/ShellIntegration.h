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
     * @brief Tells whether the shell integration setting is on.
     * @return `true` when the `shellIntegration` setting is true. The default is true.
     */
    static bool isShellIntegrated();

    /**
     * @brief Returns the folder where the shell starts.
     * @return Folder from the `startDirectory` setting. The default is `~`.
     */
    static juce::File getStartDirectory();

    /**
     * @brief Writes the integration scripts into a folder.
     *
     * The call creates the folder and its `zsh` subfolder when they are absent.
     * It writes the zsh environment file and the zsh integration file into the `zsh` subfolder.
     * It writes the bash script and the PowerShell script into the folder. It replaces files that exist.
     *
     * @param directory  Folder that receives the scripts.
     */
    static void addShellScripts (const juce::File& directory);

    /**
     * @brief Returns the environment variables that the shell receives.
     *
     * For zsh, the variables point `ZDOTDIR` to the `zsh` subfolder of @p directory. They keep the parent `ZDOTDIR` in `EVE_ORIG_ZDOTDIR`.
     * For bash, the variables point `ENV` to the bash script in @p directory and set the bash inject flags.
     * Every shell also receives `CHERE_INVOKING`.
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
     *
     * @param shell      Shell program name or absolute path. The function keys on the file name without extension.
     * @param directory  Folder that holds the integration scripts.
     * @return           Space-separated arguments for the shell. The string is empty when the shell needs none.
     */
    static juce::String getShellArguments (const juce::String& shell, const juce::File& directory);
};
