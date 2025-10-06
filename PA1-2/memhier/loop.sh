counter=1
while true; do
    output=$(./gen_test.sh 2>&1)    # capture stdout + stderr
    echo "$counter"
    echo "$output"                  # optional: print it

    cat diff.txt
    echo ""
    # First check: did the output indicate a difference?
    if [[ "$output" == *"Outputs differ. See diff.txt"* ]]; then
        echo "Outputs differ."
        # Only append config if 'hierarchy' does NOT appear anywhere in output
        # Only append config if no 'hierarchy' in output AND diff.txt contains 'memory'
    if [[ "$output" != *"hierarchy"* ]] && grep -q "memory" diff.txt; then
        echo "Appending trace.config to failed_configs.txt"
        {
        echo "=== Trace config #$counter ==="
        cat trace.config
        echo ""
        } >> failed_configs.txt
    else
        echo "Skipped appending (output contained 'hierarchy' or no 'memory')."
    fi


        # Nested check: is diff.txt longer than 12 lines?
    if [ -f diff.txt ] && [ $(wc -l < diff.txt) -gt 12 ]; then
        echo "diff.txt has more than 12 lines. EXITING :("
        break
    fi
    fi

    ((counter++))
    sleep 1
done
