#!/bin/sh
set -eu

cd "$(dirname "$0")"

ls --color=always ../../../../jam > ls.ansi

printf '\346\274\242\345\255\227\n' > wide.ansi

printf 'e\314\201\n' > grapheme.ansi

printf '\033]8;;https://jrengmusic.com\007link\033]8;;\007\n' > hyperlink.ansi

printf '\033[38:5:196mtext\033[0m\n\033[38:2:18:145:222mtext\033[0m\n' > colon.ansi

printf 'text\033[1;3' > invalid/unterminated.ansi
