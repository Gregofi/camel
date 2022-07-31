#!/bin/bash

if [[ "$#" -ne 2 ]]; then
    echo "usage: run_tests.sh compiler vm"
    exit 1;
fi

COMPILER={$1};
VM={$2};

for file in expected/*.out; do
    # string extension and path from file
    file=`basename ${file/.out/}`
    echo "Running test ${file}"

    # Run compiler
    ../Cacom/target/debug/cacom compile --input-file ${file}.cml;
    if [[ $? -ne 0 ]]; then
        echo "Test ${file} failed - Compilation failed";
        continue;
    fi;

    # Run VM
    ../Caby/build/caby execute a.out > out.tmp;
    if [[ $? -ne 0 ]]; then
        echo "Test ${file} failed - Interpreting failed";
        rm a.out;
        rm out.tmp;
        continue;
    fi;

    # Compare results
    diff out.tmp expected/${file}.out;
    if [[ $? -ne 0 ]]; then
        echo "Test ${file} failed - Different output";
    fi;
    rm a.out;
    rm out.tmp;
    echo "Test ${file} successfull :-)";
done;
