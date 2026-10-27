# EVE terminal shell integration for bash - do not edit

# Exit POSIX mode (we were launched with --posix to force $ENV sourcing)
if [[ -n "$EVE_BASH_INJECT" ]]; then
    unset EVE_BASH_INJECT

    # Restore HISTFILE (POSIX mode defaults to ~/.sh_history)
    if [[ -n "$EVE_BASH_UNEXPORT_HISTFILE" ]]; then
        export HISTFILE="$HOME/.bash_history"
        unset EVE_BASH_UNEXPORT_HISTFILE
    fi

    # Turn off POSIX mode
    set +o posix

    # Source the real startup files
    if [[ -z "$EVE_BASH_NORC" ]]; then
        if [[ -n "$EVE_BASH_RCFILE" ]]; then
            source "$EVE_BASH_RCFILE"
            unset EVE_BASH_RCFILE
        elif [[ -f "$HOME/.bashrc" ]]; then
            source "$HOME/.bashrc"
        fi
    fi
    unset EVE_BASH_NORC

    # Unset ENV so child shells start normally
    unset ENV
fi

# Install hooks
_eve_precmd() { printf '\033]133;D\007'; printf '\033]7;file://%s%s\007\033]133;A\007' "$HOSTNAME" "$PWD"; }
PS0=$'\033]133;C\007'
PROMPT_COMMAND="_eve_precmd${PROMPT_COMMAND:+;$PROMPT_COMMAND}"
