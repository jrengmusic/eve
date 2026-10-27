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
