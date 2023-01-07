#!/usr/bin/env bash
set -euo pipefail

# Execute tests from the directory that which contains the script
cd "$(dirname "$0")"

trap_exit () {
    echo "A command run from this script failed !"
}

trap trap_exit ERR

EXE_bf="$1"

swap_stdout_stderr()
{
    "$@" 3>&1 1>&2 2>&3 3>&-
}

do_test_1arg()
{
    local BF_STDOUT=`mktemp`
    local BF_STDERR=`mktemp`

    set +e
    $EXE_bf "$1" >"$BF_STDOUT" 2>"$BF_STDERR"
    local BF_EXIT_STATUS=$?
    set -e

    [ $BF_EXIT_STATUS == "$2" ] || echo "Incorrect exit status $BF_EXIT_STATUS on $1"
    diff --color=always -u $BF_STDOUT "$3"
    diff --color=always -u $BF_STDERR "$4"
}

do_test_2args()
{
    local BF_STDOUT=`mktemp`
    local BF_STDERR=`mktemp`

    set +e
    $EXE_bf "$1" "$2" >"$BF_STDOUT" 2>"$BF_STDERR"
    local BF_EXIT_STATUS=$?
    set -e

    [ $BF_EXIT_STATUS == "$3" ] || echo "Incorrect exit status $BF_EXIT_STATUS on $1 $2"
    diff --color=always -u $BF_STDOUT "$4"
    diff --color=always -u $BF_STDERR "$5"
}

do_test_3args()
{
    local BF_STDOUT=`mktemp`
    local BF_STDERR=`mktemp`

    set +e
    $EXE_bf "$1" "$2" "$3" >"$BF_STDOUT" 2>"$BF_STDERR"
    local BF_EXIT_STATUS=$?
    set -e

    [ $BF_EXIT_STATUS == "$4" ] || echo "Incorrect exit status $BF_EXIT_STATUS on $1 $2 $3"
    diff --color=always -u $BF_STDOUT "$5"
    diff --color=always -u $BF_STDERR "$6"
}

do_test_4args()
{
    local BF_STDOUT=`mktemp`
    local BF_STDERR=`mktemp`

    set +e
    $EXE_bf "$1" "$2" "$3" "$4" >"$BF_STDOUT" 2>"$BF_STDERR"
    local BF_EXIT_STATUS=$?
    set -e

    [ $BF_EXIT_STATUS == "$5" ] || echo "Incorrect exit status $BF_EXIT_STATUS on $1 $2 $3 $4"
    diff --color=always -u $BF_STDOUT "$6"
    diff --color=always -u $BF_STDERR "$7"
}

do_test_5args()
{
    local BF_STDOUT=`mktemp`
    local BF_STDERR=`mktemp`

    set +e
    $EXE_bf "$1" "$2" "$3" "$4" "$5" >"$BF_STDOUT" 2>"$BF_STDERR"
    local BF_EXIT_STATUS=$?
    set -e

    [ $BF_EXIT_STATUS == "$6" ] || echo "Incorrect exit status $BF_EXIT_STATUS on $1 $2 $3 $4 $5"
    diff --color=always -u $BF_STDOUT "$7"
    diff --color=always -u $BF_STDERR "$8"
}

do_test_9args()
{
    local BF_STDOUT=`mktemp`
    local BF_STDERR=`mktemp`

    set +e
    $EXE_bf "$1" "$2" "$3" "$4" "$5" "$6" "$7" "$8" "$9" >"$BF_STDOUT" 2>"$BF_STDERR"
    local BF_EXIT_STATUS=$?
    set -e

    [ $BF_EXIT_STATUS == "${10}" ] || echo "Incorrect exit status $BF_EXIT_STATUS on $1 $2 $3 $4 $5 $6 $7 $8 $9"
    diff --color=always -u $BF_STDOUT "${11}"
    diff --color=always -u $BF_STDERR "${12}"
}

do_test_1arg ../../examples/hello-world.bf 0 <(echo 'Hello World!') /dev/null &
do_test_1arg ../../examples/hello-world-no-comments.bf 0 <(echo 'Hello World!') /dev/null &
do_test_1arg ../../examples/emit-uppercase-alphabet.bf 0 <(printf "ABCDEFGHIJKLMNOPQRSTUVWXYZ") /dev/null &
do_test_1arg ../../examples/emit-uppercase-alphabet-no-comments.bf 0 <(printf "ABCDEFGHIJKLMNOPQRSTUVWXYZ") /dev/null &
do_test_1arg ../../examples/sierpinski.bf 0 ./outputs/sierpinski /dev/null &

# Wait for all the tests to have ended
wait

