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

#if JUCE_WINDOWS
    /**
     * @brief Unpacks the cygwin PTY helper archive into a folder, unless the folder already holds the helper.
     *
     * The build defines the hash of the helper files. The call keeps the hash in a stamp file in the folder.
     * When the stamp matches the hash and the helper exists, the call skips the unpack.
     * The call writes the stamp after an unpack, when the helper exists.
     * When more than one archive resource exists, the last one gives the result.
     *
     * @param directory  Folder that receives the unpacked files.
     * @return           Helper executable named by the first entry of the archive. An empty File when no archive resource exists or the unpack fails.
     */
    static juce::File getOrCreateCygwinPty (const juce::File& directory);
#endif

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

#if JUCE_WINDOWS
    /**
     * @brief Unpacks one archive resource into a folder, or reuses the helper that the folder holds.
     *
     * @param resourceName  BinaryData name of the archive.
     * @param directory     Folder that receives the unpacked files.
     * @param isOffloaded   `true` when the stamp matches the build hash. With the helper present, the call skips the unpack.
     * @return              Helper executable named by the first entry of the archive. An empty File when the unpack fails.
     */
    static juce::File getOrCreateCygwinPty (const char* resourceName, const juce::File& directory, bool isOffloaded);
#endif
};
