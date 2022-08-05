#!/bin/bash

if [[ "$#" -ne 2 ]]; then
    echo "usage: run_tests.sh compiler vm"
    echo "  Must be run in the tests directory"
    exit 1;
fi

COMPILER=${1};
VM=${2};
SUCCESS=0;

mkdir -p out

for file in expected/*.exp; do
    # string extension and path from file
    file=`basename ${file/.exp/}`
    echo "Running test ${file}"

    # Run compiler
    ${COMPILER} compile --input-file ${file}.cml;
    if [[ $? -ne 0 ]]; then
        echo "Test ${file} failed - Compilation failed";
        SUCCESS=1;
        continue;
    fi;

    # Run VM
    ${VM} execute a.out > out/${file}.out;
    if [[ $? -ne 0 ]]; then
        echo "Test ${file} failed - Interpreting failed";
        SUCCESS=1
        rm a.out;
        rm out.tmp;
        continue;
    fi;

    # Compare results
    diff out/${file}.out expected/${file}.exp;
    if [[ $? -ne 0 ]]; then
        SUCCESS=1
        echo "Test ${file} failed - Different output";
    fi;
    rm a.out;
    echo "Test ${file} successfull :-)";
done;

exit ${SUCCESS};
