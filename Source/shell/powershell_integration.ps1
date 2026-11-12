# EVE terminal shell integration for PowerShell - do not edit

# Source user profile if it exists
if (Test-Path $PROFILE) { . $PROFILE }

# Save original prompt
$_eve_orig_prompt = $function:prompt

# Override prompt (precmd equivalent)
function prompt {
    $exitCode = $global:LASTEXITCODE
    # OSC 133 D - command finished
    $Host.UI.Write("$([char]27)]133;D;$exitCode`a")
    # OSC 7 - current working directory
    $directory = $PWD.ProviderPath -replace '\\','/'
    $directory = if ($directory.StartsWith('//')) { $directory.Substring(2) } else { "/$directory" }
    $Host.UI.Write("$([char]27)]7;file://$(_eve_encode $directory)`a")
    # OSC 133 A - prompt start
    $Host.UI.Write("$([char]27)]133;A`a")
    # Run original prompt
    $result = & $_eve_orig_prompt
    # OSC 133 B - prompt end
    $Host.UI.Write("$([char]27)]133;B`a")
    $result
}

# Override Enter key (preexec equivalent)
if (Get-Module PSReadLine -ErrorAction SilentlyContinue) {
    Set-PSReadLineKeyHandler -Key Enter -ScriptBlock {
        $line = $null
        $cursor = $null
        [Microsoft.PowerShell.PSConsoleReadLine]::GetBufferState([ref]$line, [ref]$cursor)
        # OSC 133 C - command start
        $Host.UI.Write("$([char]27)]133;C;cmdline_url=$(_eve_encode $line)`a")
        [Microsoft.PowerShell.PSConsoleReadLine]::AcceptLine()
    }
}

function _eve_encode {
    param($Text)
    [regex]::Replace($Text, '[%;#?\x00-\x1F\x7F]', { param($match) '%{0:X2}' -f [int][char]$match.Value })
}

function eve {
    param($Subcommand, $Argument, $Options)
    switch ($Subcommand) {
        'command' { $Host.UI.Write("$([char]27)]1337;EVE;command;name=$(_eve_encode $Argument)`a") }
        'preview' {
            $path = if ($Argument) { $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Argument) } else { '' }
            $width = if ($env:FZF_PREVIEW_COLUMNS) { $env:FZF_PREVIEW_COLUMNS } else { '100%' }
            $height = if ($env:FZF_PREVIEW_LINES) { $env:FZF_PREVIEW_LINES } else { '100%' }
            $arguments = if ($Options) { ";$Options" } else { '' }
            $Host.UI.Write("$([char]27)]1337;EVE;preview;inline=1;width=$width;height=$height$arguments;path=$(_eve_encode $path)`a")
        }
        default { [Console]::Error.WriteLine("eve: unknown command '$Subcommand'; use command or preview"); $global:LASTEXITCODE = 1 }
    }
}
