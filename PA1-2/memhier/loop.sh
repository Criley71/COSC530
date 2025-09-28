#!/bin/bash
counter=1
while true; do
    output=$(./gen_test.sh)    # capture output
    echo "$counter"
    echo "$output"               # optional: print it
    if [[ "$output" == *"Outputs differ. See diff.txt"* ]]; then
        echo "Outputs differ. See diff.txt EXITING :("
        break
    fi
    ((counter++))
    sleep 1
done
