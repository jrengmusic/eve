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
_eve_precmd() { printf '\033]133;D\007'; printf '\033]7;file://%s%s\007\033]133;A\007' "$HOSTNAME" "$(_eve_encode "$PWD")"; }
PS0='$(_eve_preexec > /dev/tty)'
PROMPT_COMMAND="_eve_precmd${PROMPT_COMMAND:+;$PROMPT_COMMAND}"

_eve_encode() {
    local LC_ALL=C
    local text="$1" char encoded='' i
    for (( i = 0; i < ${#text}; ++i )); do
        char="${text:i:1}"
        case "$char" in
            '%'|';'|[[:cntrl:]]) encoded+="$(printf '%%%02X' "'$char")" ;;
            *) encoded+="$char" ;;
        esac
    done
    printf '%s' "$encoded"
}

_eve_preexec() {
    local last_cmd
    last_cmd=$(HISTTIMEFORMAT= builtin history 1)
    last_cmd="${last_cmd#*[[:digit:]]*[[:space:]]}"
    last_cmd="${last_cmd#"${last_cmd%%[![:space:]]*}"}"
    printf '\033]133;C;cmdline_url=%s\007' "$(_eve_encode "$last_cmd")"
}

eve() {
    case "$1" in
        command) printf '\033]1337;EVE;command;name=%s\007' "$(_eve_encode "$2")" ;;
        preview) printf '\033]1337;EVE;preview;path=%s\007' "$(_eve_encode "${2:+$(realpath "$2")}")" ;;
        image) printf '\033]1337;EVE;image;:%s\007' "$(base64 < "$(realpath "$2")" | tr -d '\n')" ;;
        *) printf "eve: unknown command '%s'; use command, preview or image\n" "$1" >&2; return 1 ;;
    esac
}
