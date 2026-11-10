#include "ShellIntegration.h"

static constexpr std::string_view zshDirectory { "zsh" };
static constexpr std::string_view zshIntegrationFile { "eve-integration" };
static constexpr std::string_view zshEnvironmentFile { ".zshenv" };

bool ShellIntegration::isShellIntegrated()
{
    const auto text { jam::ParameterManager::getInstance()->getDefaultValueForSettings (jam::Format::toTag (Id::shellIntegration), Id::tokenTrue.toString()) };

    return juce::Identifier { text } == Id::tokenTrue;
}

juce::File ShellIntegration::getStartDirectory()
{
    return juce::File { jam::ParameterManager::getInstance()->getDefaultValueForSettings (jam::Format::toTag (Id::startDirectory), juce::File::getSpecialLocation (juce::File::userHomeDirectory).getFullPathName()) };
}

void ShellIntegration::addShellScripts (const juce::File& directory)
{
    const auto zshFolder { directory.getChildFile (juce::String { zshDirectory.data() }) };
    const BinaryData::Raw zshEnvironment { files::zshEnvironment };
    const BinaryData::Raw zshIntegration { files::zshIntegration };
    const BinaryData::Raw bashIntegration { files::bashIntegration };
    const BinaryData::Raw powershellIntegration { files::powershellIntegration };
    const BinaryData::Raw shellCommand { files::shellCommand };

    directory.createDirectory();
    zshFolder.createDirectory();
    zshFolder.getChildFile (juce::String { zshEnvironmentFile.data() }).replaceWithData (zshEnvironment.data, static_cast<size_t> (zshEnvironment.size));
    zshFolder.getChildFile (juce::String { zshIntegrationFile.data() }).replaceWithData (zshIntegration.data, static_cast<size_t> (zshIntegration.size));
    directory.getChildFile (files::bashIntegration).replaceWithData (bashIntegration.data, static_cast<size_t> (bashIntegration.size));
    directory.getChildFile (files::powershellIntegration).replaceWithData (powershellIntegration.data, static_cast<size_t> (powershellIntegration.size));

    const auto commandFile { directory.getChildFile (files::shellCommand) };

    commandFile.replaceWithData (shellCommand.data, static_cast<size_t> (shellCommand.size));
    commandFile.setExecutePermission (true);
}

juce::StringPairArray ShellIntegration::getShellEnvironment (const juce::String& shell, const juce::File& directory)
{
    static const auto shellEnvironments ([] {
        jam::Function::Map<juce::String, jam::Function::pointerCapacity, juce::StringPairArray (const juce::File&)> shellEnvironments;

        shellEnvironments.add ("zsh",
            [] (const juce::String&, const juce::File& directory)
            {
                juce::StringPairArray environment;
                const auto parentZdotdir { juce::SystemStats::getEnvironmentVariable ("ZDOTDIR", {}) };

                if (parentZdotdir.isNotEmpty())
                    environment.set ("EVE_ORIG_ZDOTDIR", parentZdotdir);

                environment.set ("ZDOTDIR", directory.getChildFile (juce::String { zshDirectory.data() }).getFullPathName());
                return environment;
            });
        shellEnvironments.add ("bash",
            [] (const juce::String&, const juce::File& directory)
            {
                juce::StringPairArray environment;

                environment.set ("ENV", directory.getChildFile (files::bashIntegration).getFullPathName());
                environment.set ("EVE_BASH_INJECT", "1");
                environment.set ("EVE_BASH_UNEXPORT_HISTFILE", "1");
                return environment;
            });
        return shellEnvironments;
    }());

    const auto key { juce::File (shell).getFileNameWithoutExtension() };

    auto environment { shellEnvironments.contains (key) ? shellEnvironments.get (key, directory) : juce::StringPairArray {} };

    environment.set ("CHERE_INVOKING", "1");
    return environment;
}

juce::String ShellIntegration::getShellArguments (const juce::String& shell, const juce::File& directory)
{
    static const auto shellArguments ([] {
        jam::Function::Map<juce::String, jam::Function::pointerCapacity, juce::String (const juce::File&)> shellArguments;
        const auto getPowershellArguments { [] (const juce::String&, const juce::File& directory)
                                         {
                                             return "-NoLogo -NoProfile -NoExit -Command \". '" + directory.getChildFile (files::powershellIntegration).getFullPathName() + "'\"";
                                         } };

        shellArguments.add ("bash", [] (const juce::String&, const juce::File&) { return juce::String { "--posix" }; });
        shellArguments.add ("powershell", getPowershellArguments);
        shellArguments.add ("pwsh", getPowershellArguments);
        return shellArguments;
    }());

    const auto key { juce::File (shell).getFileNameWithoutExtension() };

    if (shellArguments.contains (key)) return shellArguments.get (key, directory);

    return {};
}

void ShellIntegration::appendPath (std::string& output, const juce::String& shell, std::string_view path)
{
    static constexpr std::string_view posixEscapedQuote { "'\\''" };
    static constexpr std::string_view powershellEscapedQuote { "''" };

    static const auto quotedPaths ([] {
        jam::Function::Map<juce::String, jam::Function::pointerCapacity, void (std::string&, std::string_view)> quotedPaths;
        const auto appendPowershellPath { [] (const juce::String&, std::string& output, std::string_view path)
                                          {
                                              jam::Format::appendQuoted (output, path, static_cast<char> (Chars::singleQuote), powershellEscapedQuote);
                                          } };

        quotedPaths.add ("powershell", appendPowershellPath);
        quotedPaths.add ("pwsh", appendPowershellPath);
        return quotedPaths;
    }());

    const auto key { juce::File (shell).getFileNameWithoutExtension() };

    if (quotedPaths.contains (key)) quotedPaths.get (key, output, path);
    else jam::Format::appendQuoted (output, path, static_cast<char> (Chars::singleQuote), posixEscapedQuote);
}

bool ShellIntegration::hasControlByte (std::string_view path)
{
    return std::any_of (path.begin(), path.end(), [] (char byte) { return std::iscntrl (static_cast<unsigned char> (byte)) != 0; });
}
