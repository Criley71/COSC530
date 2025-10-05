counter=1
while true; do
    output=$(./gen_test.sh)    # capture output
    echo "$counter"
    echo "$output"               # optional: print it

    # First check: did the output indicate a difference?
    if [[ "$output" == *"Outputs differ. See diff.txt"* ]]; then
        echo "Memory counts differ."

        # Append the trace.config to a log file
        echo "=== Trace config #$counter ===" >> failed_configs.txt
        cat trace.config >> failed_configs.txt
        echo "" >> failed_configs.txt  # blank line for separation

        # Nested check: is diff.txt longer than 12 lines?
        if [ -f diff.txt ] && [ $(wc -l < diff.txt) -gt 12 ]; then
            echo "diff.txt has more than 12 lines. EXITING :("
            break
        fi
    fi

    ((counter++))
    sleep 1
done
