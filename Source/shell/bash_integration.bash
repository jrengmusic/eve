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

# The eve command lies beside this file. It also gives _eve_encode to the hooks
_eve_directory="${BASH_SOURCE[0]%/*}"
PATH="$_eve_directory:$PATH"
source "$_eve_directory/eve"
unset _eve_directory

if command -v cygpath > /dev/null 2>&1; then
    _eve_cwd() { printf '/%s' "$(cygpath -m "$PWD")"; }
else
    _eve_cwd() { printf '%s' "$PWD"; }
fi

# Install hooks
_eve_precmd() { printf '\033]133;D\007'; printf '\033]7;file://%s%s\007\033]133;A\007' "$HOSTNAME" "$(_eve_encode "$(_eve_cwd)")"; }
PS0='$(_eve_preexec > /dev/tty)'
PROMPT_COMMAND="_eve_precmd${PROMPT_COMMAND:+;$PROMPT_COMMAND}"

_eve_preexec() {
    local last_cmd
    last_cmd=$(HISTTIMEFORMAT= builtin history 1)
    last_cmd="${last_cmd#*[[:digit:]]*[[:space:]]}"
    last_cmd="${last_cmd#"${last_cmd%%[![:space:]]*}"}"
    printf '\033]133;C;cmdline_url=%s\007' "$(_eve_encode "$last_cmd")"
}
