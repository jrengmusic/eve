# EVE terminal shell integration hooks
# Sourced by .zshenv for interactive shells.

# The eve command lies in the parent of this directory
path=("${${(%):-%x}:h:h}" $path)

_eve_precmd() {
    printf '\033]133;D\007'
    printf '\033]7;file://%s%s\007\033]133;A\007' "$HOST" "$(_eve_encode "$PWD")"
}

_eve_preexec() {
    printf '\033]133;C;cmdline_url=%s\007' "$(_eve_encode "$1")"
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
