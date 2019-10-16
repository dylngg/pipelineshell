#!/bin/bash
# Tests the plsh program. Should be executed in test directory.
#
# Usage:
# ./test.sh TESTFILE ... (Tests the given test files)
# ./test.sh (Tests all the files in the current directory)

die() {
    echo "$1" > /dev/stderr
    exit 1
}

delete=true
test_dir=.
tests=$@
dir=`mktemp -d`
if [ "$tests" == "" ]; then
    tests=$test_dir/*.plsh
fi

exe="../plsh"
if [ ! -f $exe ]; then
    die "Not in tests directory"
fi

for file in $tests; do
    testname=`basename $file .plsh`
    ref_out=$dir/$testname.ref
    test_out=$test_dir/$testname.out
    if [ ! -f $file ]; then
        die "Test could not be found: $file"
    elif [ ! -f $test_out ]; then
        die "Test .out file could not be found: $file"
    fi

    printf "Testing $testname: "
    $exe $file > $ref_out

    if [ "`diff $test_out $ref_out`" != "" ]; then
        printf "✘\n"
        diff -d -y $test_out $ref_out
        ! $delete && echo "Output can be found at $dir"
        exit 1
    else
        printf "✓\n"
    fi
done

# Delete the directory if no errors
if $delete; then
    rm -r $dir
fi
