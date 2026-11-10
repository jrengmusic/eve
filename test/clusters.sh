#!/bin/zsh

clear

read -r rows columns <<< "$(stty size)"

delay=${1:-0.1}
passed=0
failed=0
typeset -a failures
section=""

trap 'printf "\e[4l"; stty echo' EXIT

stty -echo

printf '\e[?7h'

typeset -a sequences

sequences=(
    "31efb88fe283a3 keycap 1 qualified"
    "23efb88fe283a3 keycap # qualified"
    "2aefb88fe283a3 keycap * qualified"
    "30efb88fe283a3 keycap 0 qualified"
    "39efb88fe283a3 keycap 9 qualified"
    "31e283a3 keycap 1 unqualified"
    "31efb88ee283a3 keycap 1 text selector"
    "65cc81 e plus combining acute"
    "e29da4efb88f heart plus emoji selector"
    "e29da4efb88e heart plus text selector"
    "f09f9880efb88e smiley plus text selector"
    "f09f91a8e2808df09f91a9e2808df09f91a7 family with zero width joiners"
    "f09f87aef09f87a9 regional indicator pair"
    "f09f918df09f8fbd thumbs up plus skin tone"
    "e0a495e0a58d devanagari ka plus virama"
    "f09f8fb4f3a081a7f3a081a2f3a081a5f3a081aef3a081a7f3a081bf tag sequence"
)

write_hex()
{
    local hex=$1
    local bytes=""

    while (( ${#hex} > 0 ))
    do
        bytes+="\\x${hex[1,2]}"
        hex=${hex[3,-1]}
    done

    printf "$bytes"
}

write_split()
{
    local hex=$1
    local offset=$2

    write_hex "${hex[1,offset]}"
    sleep $delay
    write_hex "${hex[offset+1,-1]}"
}

get_position()
{
    local report

    printf '\e[6n'
    IFS= read -rs -t 2 -d R report

    if [[ -z $report ]]
    then
        print -u2 "no reply to the cursor position query"
        exit 2
    fi

    report=${report#*\[}
    position_row=${report%%;*}
    position_column=${report##*;}
}

begin_case()
{
    printf '\n\n\n\e[3A\r\e[2K'
    get_position
    start_row=$position_row
}

end_case()
{
    get_position
    case_rows=$(( position_row - start_row ))
    case_columns=$(( position_column - 1 ))
}

record()
{
    local verdict=$1
    local colour

    case $verdict in
        pass) colour=32; (( passed++ )) ;;
        fail) colour=31; (( failed++ )); failures+=("${section} | $2  $3") ;;
        info) colour=33 ;;
    esac

    printf '\e[%dm%-4s\e[0m %s  %s\n' $colour ${(U)verdict} "$2" "$3"
}

print_section()
{
    section=$1
    printf '\n\e[1m=== %s ===\e[0m\n' "$1"
}

measure()
{
    local hex=$1
    local offset=$2
    local fillers=$3
    local setup=$4
    local teardown=$5

    begin_case
    printf "$setup"

    if (( fillers > 0 ))
    then
        printf 'x%.0s' {1..$fillers}
    fi

    if (( offset == 0 ))
    then
        write_hex $hex
    else
        write_split $hex $offset
    fi

    end_case
    printf "$teardown"
}

compare()
{
    local label=$1
    local expected_rows=$2
    local expected_columns=$3
    local detail="rows=${case_rows} columns=${case_columns} expected rows=${expected_rows} columns=${expected_columns}"

    printf '\n'

    if (( case_rows == expected_rows && case_columns == expected_columns ))
    then
        record pass "$label" "$detail"
    else
        record fail "$label" "$detail"
    fi
}

check_columns()
{
    local label=$1
    local hex=$2
    local expected_columns=$3

    measure $hex 0 0 "" ""
    compare "$label" 0 $expected_columns
}

check_splits()
{
    local hex=$1
    local label=$2
    local fillers=$3
    local setup=$4
    local teardown=$5
    local whole_rows whole_columns offset

    measure $hex 0 $fillers "$setup" "$teardown"
    whole_rows=$case_rows
    whole_columns=$case_columns
    printf '\n'

    for (( offset = 2; offset < ${#hex}; offset += 2 ))
    do
        measure $hex $offset $fillers "$setup" "$teardown"
        compare "$label split after byte $(( offset / 2 )) fillers=$fillers" $whole_rows $whole_columns
    done
}

observe()
{
    local label=$1
    shift
    local part

    begin_case

    for part in "$@"
    do
        printf "$part"
        sleep $delay
    done

    end_case
    printf '\n'
    record info "$label" "rows=${case_rows} columns=${case_columns}"
}

get_first_length()
{
    local lead=$(( 16#${1[1,2]} ))

    if (( lead < 128 ))
    then
        first_length=1
    elif (( lead < 224 ))
    then
        first_length=2
    elif (( lead < 240 ))
    then
        first_length=3
    else
        first_length=4
    fi
}

print_section "width of a whole write (jam_AttributedChar.cpp:3319-3332)"

check_columns "plain digit" 31 1
check_columns "keycap qualified" 31efb88fe283a3 2
check_columns "keycap unqualified" 31e283a3 2
check_columns "keycap text selector" 31efb88ee283a3 2
check_columns "e plus combining acute" 65cc81 1
check_columns "heart plus emoji selector" e29da4efb88f 2
check_columns "heart plus text selector" e29da4efb88e 1
check_columns "smiley" f09f9880 2
check_columns "smiley plus text selector" f09f9880efb88e 2

print_section "split write equals whole write, every byte boundary, start of line"

for entry in $sequences
do
    check_splits ${entry%% *} "${entry#* }" 0 "" ""
done

print_section "split write equals whole write, one column before the right edge"

for entry in $sequences
do
    check_splits ${entry%% *} "${entry#* }" $(( columns - 1 )) "" ""
done

print_section "split write equals whole write, two columns before the right edge"

for entry in $sequences
do
    check_splits ${entry%% *} "${entry#* }" $(( columns - 2 )) "" ""
done

print_section "split write equals whole write, insert mode"

for entry in $sequences
do
    check_splits ${entry%% *} "${entry#* }" 0 'abcdef\r\e[4h' '\e[4l'
done

print_section "cursor moves between the two writes (no verdict)"

observe "digit, carriage return, selector and keycap mark" '1' '\r' '\xef\xb8\x8f\xe2\x83\xa3'
observe "digit, cursor to column 10, selector and keycap mark" '1' '\e[1;10H' '\xef\xb8\x8f\xe2\x83\xa3'
observe "digit, colour change, selector and keycap mark" '1' '\e[31m' '\xef\xb8\x8f\xe2\x83\xa3\e[0m'
observe "digit, erase to end of line, selector and keycap mark" '1' '\e[K' '\xef\xb8\x8f\xe2\x83\xa3'

print_section "rendering, whole write then split after the first character"

for entry in $sequences
do
    get_first_length ${entry%% *}
    begin_case
    printf '|'
    write_hex ${entry%% *}
    printf '|  |'
    write_split ${entry%% *} $(( first_length * 2 ))
    printf '|  %s\n' "${entry#* }"
done

if (( failed > 0 ))
then
    print_section "failed cases"
    printf '%s\n' "${failures[@]}"
fi

printf '\n%d passed, %d failed\n' $passed $failed

exit $(( failed > 0 ))
