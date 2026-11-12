# EVE terminal shell integration hooks
# Sourced by .zshenv for interactive shells.

# The eve command lies in the parent of this directory. It also gives _eve_encode and _eve_cwd to the hooks
path=("${${(%):-%x}:h:h}" $path)
source "${${(%):-%x}:h:h}/eve"

_eve_precmd() {
    printf '\033]133;D\007'
    printf '\033]7;file://%s\007\033]133;A\007' "$(_eve_encode "$(_eve_cwd)")"
}

_eve_preexec() {
    printf '\033]133;C;cmdline_url=%s\007' "$(_eve_encode "$1")"
}

# Use add-zsh-hook (robust against array-clobber by frameworks)
autoload -Uz add-zsh-hook
add-zsh-hook precmd _eve_precmd
add-zsh-hook preexec _eve_preexec
