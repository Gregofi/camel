#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

if [[ "$#" -ne 2 ]]; then
    echo "usage: run_tests.sh compiler vm"
    echo "  Must be run in the tests directory"
    exit 1;
fi

COMPILER=${1};
VM=${2};
SUCCESS=0;
TOTAL=0;

mkdir -p out

for file in expected/*.exp; do
    ((TOTAL+=1));
    # string extension and path from file
    file=`basename ${file/.exp/}`
    echo "Running test ${file}"

    # Run compiler
    ${COMPILER} compile --input-file ${file}.cml;
    if [[ $? -ne 0 ]]; then
        printf "${RED}Test ${file} failed - Compilation failed${NC}\n";
        continue;
    fi;

    # Run VM
    ${VM} execute a.out > out/${file}.out;
    if [[ $? -ne 0 ]]; then
        printf "${RED}Test ${file} failed - Interpreting failed${NC}\n";
        rm a.out;
        rm out.tmp;
        continue;
    fi;

    # Compare results
    diff out/${file}.out expected/${file}.exp;
    if [[ $? -ne 0 ]]; then
        printf "${RED}Test ${file} failed - Different output${NC}\n";
        rm a.out;
        continue;
    fi;
    rm a.out;
    ((SUCCESS+=1))
    printf "${GREEN}Test ${file} successfull :-)${NC}\n";
done;


if [[ SUCCESS -ne TOTAL ]]; then
    printf "\nTests result: ${RED}${SUCCESS}/${TOTAL} - Testing FAILED\n${NC}";
    exit 1;
fi

printf "${GREEN}\nAll tests passed!\n${NC}";
exit 0;
