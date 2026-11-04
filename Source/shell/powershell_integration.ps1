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
    $Host.UI.Write("$([char]27)]7;file://$([System.Net.Dns]::GetHostName())/$(_eve_encode ($PWD.Path -replace '\\','/'))`a")
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
    [regex]::Replace($Text, '[%;\x00-\x1F\x7F]', { param($match) '%{0:X2}' -f [int][char]$match.Value })
}

function eve {
    param($Subcommand, $Argument)
    switch ($Subcommand) {
        'command' { $Host.UI.Write("$([char]27)]1337;EVE;command;name=$(_eve_encode $Argument)`a") }
        'preview' {
            $path = if ($Argument) { $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Argument) } else { '' }
            $Host.UI.Write("$([char]27)]1337;EVE;preview;path=$(_eve_encode $path)`a")
        }
        'image' {
            $path = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Argument)
            $Host.UI.Write("$([char]27)]1337;EVE;image;:$([Convert]::ToBase64String([IO.File]::ReadAllBytes($path)))`a")
        }
        default { [Console]::Error.WriteLine("eve: unknown command '$Subcommand'; use command, preview or image"); $global:LASTEXITCODE = 1 }
    }
}
