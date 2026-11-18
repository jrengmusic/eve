#!/bin/zsh

clear

delay=${1:-0.5}
passed=0
failed=0
typeset -a failures
section=""

trap 'kill -KILL $(jobs -p) 2> /dev/null' EXIT

setopt monitor

if [[ ! -o monitor ]]
then
    print -u2 "job control is off: this shell has no controlling terminal"
    exit 2
fi

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

expect_state()
{
    local label=$1
    local job=$2
    local expected=$3
    local state

    sleep $delay
    state=${${jobstates[$job]}%%:*}

    if [[ $state == $expected ]]
    then
        record pass "$label" "job $job is $state"
    else
        record fail "$label" "job $job is '${state}', expected $expected"
    fi
}

print_section "background job"

sleep 30 &
job_pid=$!

expect_state "a background job runs" 1 running

shell_group=$(< /proc/$$/pgid)
job_group=$(< /proc/$job_pid/pgid)

if (( job_group != shell_group ))
then
    record pass "the job has its own process group" "shell group ${shell_group}, job group ${job_group}"
else
    record fail "the job has its own process group" "shell group ${shell_group}, job group ${job_group}"
fi

print_section "stop and continue"

kill -STOP %1
expect_state "SIGSTOP suspends the job" 1 suspended

kill -CONT %1
expect_state "SIGCONT resumes the job" 1 running

kill -KILL %1
wait %1 2> /dev/null

print_section "foreground handoff"

sleep 1 &
fg %1 > /dev/null
foreground_status=$?

if (( foreground_status == 0 ))
then
    record pass "fg gives the terminal to the job and takes it back" "status ${foreground_status}"
else
    record fail "fg gives the terminal to the job and takes it back" "status ${foreground_status}"
fi

print_section "background read"

cat &
expect_state "a background job that reads the terminal is suspended" 1 suspended

kill -KILL %1
wait %1 2> /dev/null

if (( failed > 0 ))
then
    print_section "failed cases"
    printf '%s\n' "${failures[@]}"
fi

printf '\n%d passed, %d failed\n' $passed $failed
printf '\nBy hand: run sleep 100, press ^Z, run jobs, bg, fg, then press ^C.\n'

exit $(( failed > 0 ))
