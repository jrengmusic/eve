# EVE terminal shell integration hooks
# Sourced by .zshenv for interactive shells.

_eve_precmd() {
    printf '\033]133;D\007'
    printf '\033]7;file://%s%s\007\033]133;A\007' "$HOST" "$PWD"
}

_eve_preexec() {
    printf '\033]133;C\007'
}

# Use add-zsh-hook (robust against array-clobber by frameworks)
autoload -Uz add-zsh-hook
add-zsh-hook precmd _eve_precmd
add-zsh-hook preexec _eve_preexec

_eve_encode() {
    local LC_ALL=C
    local text="$1" char encoded='' i
    for (( i = 1; i <= ${#text}; ++i )); do
        char="${text[i]}"
        case "$char" in
            '%'|';'|[[:cntrl:]]) encoded+="$(printf '%%%02X' "'$char")" ;;
            *) encoded+="$char" ;;
        esac
    done
    printf '%s' "$encoded"
}

eve() {
    case "$1" in
        command) printf '\033]1337;EVE;command;name=%s\007' "$(_eve_encode "$2")" ;;
        preview) printf '\033]1337;EVE;preview;path=%s\007' "$(_eve_encode "${2:+${2:a}}")" ;;
        image) printf '\033]1337;EVE;image;:%s\007' "$(base64 < "${2:a}" | tr -d '\n')" ;;
        *) printf "eve: unknown command '%s'; use command, preview or image\n" "$1" >&2; return 1 ;;
    esac
}
