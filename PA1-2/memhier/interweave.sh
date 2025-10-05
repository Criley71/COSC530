#!/bin/bash

# 1️⃣ Extract "replaced" lines from out.txt, only between first two pipes
awk -F'|' '/writing/ {gsub(/^ +| +$/,"",$2); print $2}' out.txt > tmp_out.txt

# 2️⃣ Extract only lines containing "writing" from trace.log
grep "writing" trace.log > tmp_trace.txt

# 3️⃣ Interleave: 1 line from tmp_out.txt, 2 lines from tmp_trace.txt, blank line after
awk '
{
    a[++i] = $0
}
END {
    j=0
    while ((getline line < "tmp_trace.txt") > 0) { b[++j] = line }
    p1=1; p2=1
    while (p1<=i || p2<=j) {
        if(p1<=i) print a[p1++]
        if(p2<=j) print b[p2++]   # only one line from trace.log now
        print ""  # blank line after each block
    }
}
' tmp_out.txt > combined.txt


# 4️⃣ Clean up temporary files
#rm tmp_out.txt tmp_trace.txt
