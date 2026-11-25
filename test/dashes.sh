#!/bin/zsh

clear

typeset -a dashes fallbacks styles

dashes=(2014 2013 2026)
fallbacks=(2015 2030 2031 2116 0B9C 0BB3 0BA3)
styles=(0 1 3 "1;3")

print_title()
{
    printf '\n=== %s ===\n%s\n' "$1" "$2"
}

get_glyph()
{
    printf -v REPLY "\\U0000$1"
}

print_row()
{
    local glyphs="|" ruler="|" code

    for code in "$@"
    do
        get_glyph $code
        glyphs+="${REPLY}|"
        ruler+="X|"
    done

    printf '%s\n%s\n' "$glyphs" "$ruler"
}

print_text()
{
    local code

    for code in "$@"
    do
        get_glyph $code
        printf 'U+%s  use%sotherwise  H%sH  o%so%so\n' $code $REPLY $REPLY $REPLY $REPLY
    done
}

print_joined()
{
    local code

    for code in "$@"
    do
        get_glyph $code
        printf 'U+%s  H%s%s%sH\n        HXXXH\n' $code $REPLY $REPLY $REPLY
    done
}

print_inverse()
{
    local code

    for code in "$@"
    do
        get_glyph $code
        printf 'U+%s  H\e[7m%s\e[0mH  \e[7mH\e[0m%s\e[7mH\e[0m\n' $code $REPLY $REPLY
    done
}

print_styles()
{
    local style

    for style in $styles
    do
        printf '\e[%sm' $style
        print_text "$@"
        printf '\e[0m'
    done
}

print_title "1. Text" "Expect: the glyph stays between its two letters. It covers no part of a letter."
print_text $dashes

print_title "2. One cell" "Expect: each | lines up with the | of the ruler."
print_row $dashes

print_title "3. Joined" "Expect: three glyphs of equal width between the two H. The last H lines up with the ruler."
print_joined $dashes

print_title "4. Inverse cell" "Expect: the glyph stays inside its own cell. No part of it is in the cell of an H."
print_inverse $dashes

print_title "5. Styles" "Expect: regular, bold, italic and bold italic. Each one is the same as section 1."
print_styles $dashes

print_title "6. Fallback control" "Expect: the same as sections 1 to 5. Display Mono has none of these glyphs, so a fallback face draws them."
print_text $fallbacks
print_row $fallbacks
print_joined $fallbacks
print_inverse $fallbacks
print_styles $fallbacks
