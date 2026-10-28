#include "ShellIntegration.h"

static constexpr std::string_view zshDirectory { "zsh" };
static constexpr std::string_view zshIntegrationFile { "eve-integration" };
static constexpr std::string_view zshEnvironmentFile { ".zshenv" };
static constexpr std::string_view bashIntegrationFile { "bash_integration.bash" };
static constexpr std::string_view powershellIntegrationFile { "powershell_integration.ps1" };

bool ShellIntegration::isShellIntegrated()
{
    const auto text { jam::ParameterManager::getInstance()->getDefaultValueForSettings (Id::toTag (Id::shellIntegration), Id::tokenTrue.toString()) };

    return juce::Identifier { text } == Id::tokenTrue;
}

juce::File ShellIntegration::getStartDirectory()
{
    return juce::File { jam::ParameterManager::getInstance()->getDefaultValueForSettings (Id::toTag (Id::startDirectory), "~") };
}

void ShellIntegration::addShellScripts (const juce::File& directory)
{
    const auto zshFolder { directory.getChildFile (juce::String { zshDirectory.data() }) };
    const BinaryData::Raw zshEnvironment { files::zshEnvironment };
    const BinaryData::Raw zshIntegration { files::zshIntegration };
    const BinaryData::Raw bashIntegration { files::bashIntegration };
    const BinaryData::Raw powershellIntegration { files::powershellIntegration };

    directory.createDirectory();
    zshFolder.createDirectory();
    zshFolder.getChildFile (juce::String { zshEnvironmentFile.data() }).replaceWithData (zshEnvironment.data, static_cast<size_t> (zshEnvironment.size));
    zshFolder.getChildFile (juce::String { zshIntegrationFile.data() }).replaceWithData (zshIntegration.data, static_cast<size_t> (zshIntegration.size));
    directory.getChildFile (juce::String { bashIntegrationFile.data() }).replaceWithData (bashIntegration.data, static_cast<size_t> (bashIntegration.size));
    directory.getChildFile (juce::String { powershellIntegrationFile.data() }).replaceWithData (powershellIntegration.data, static_cast<size_t> (powershellIntegration.size));
}

juce::StringPairArray ShellIntegration::getShellEnvironment (const juce::String& shell, const juce::File& directory)
{
    static const jam::Function::Map<juce::String, juce::StringPairArray> shellEnvironments { [] {
        jam::Function::Map<juce::String, juce::StringPairArray> table;

        table.add<const juce::File&> ("zsh",
            [] (const juce::File& directory)
            {
                juce::StringPairArray environment;
                const auto parentZdotdir { juce::SystemStats::getEnvironmentVariable ("ZDOTDIR", {}) };

                if (parentZdotdir.isNotEmpty())
                    environment.set ("EVE_ORIG_ZDOTDIR", parentZdotdir);

                environment.set ("ZDOTDIR", directory.getChildFile (juce::String { zshDirectory.data() }).getFullPathName());
                return environment;
            });
        table.add<const juce::File&> ("bash",
            [] (const juce::File& directory)
            {
                juce::StringPairArray environment;

                environment.set ("ENV", directory.getChildFile (juce::String { bashIntegrationFile.data() }).getFullPathName());
                environment.set ("EVE_BASH_INJECT", "1");
                environment.set ("EVE_BASH_UNEXPORT_HISTFILE", "1");
                return environment;
            });
        return table;
    }() };

    const auto key { juce::File (shell).getFileNameWithoutExtension() };

    auto environment { shellEnvironments.contains (key) ? shellEnvironments.get (key, directory) : juce::StringPairArray {} };

    environment.set ("CHERE_INVOKING", "1");
    return environment;
}

juce::String ShellIntegration::getShellArguments (const juce::String& shell, const juce::File& directory)
{
    static const jam::Function::Map<juce::String, juce::String> shellArguments { [] {
        jam::Function::Map<juce::String, juce::String> table;
        const auto getPowershellArguments { [] (const juce::File& directory)
                                         {
                                             return "-NoLogo -NoProfile -NoExit -Command \". '" + directory.getChildFile (juce::String { powershellIntegrationFile.data() }).getFullPathName() + "'\"";
                                         } };

        table.add<const juce::File&> ("bash", [] (const juce::File&) { return juce::String { "--posix" }; });
        table.add<const juce::File&> ("powershell", getPowershellArguments);
        table.add<const juce::File&> ("pwsh", getPowershellArguments);
        return table;
    }() };

    const auto key { juce::File (shell).getFileNameWithoutExtension() };

    if (shellArguments.contains (key)) return shellArguments.get (key, directory);

    return {};
}
