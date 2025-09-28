#!/bin/bash

# Helper: generate a random power of two within a range
pow2() {
    local min=$1
    local max=$2
    local vals=()
    local val=1
    while [ $val -le $max ]; do
        if [ $val -ge $min ]; then
            vals+=($val)
        fi
        val=$((val * 2))
    done
    echo "${vals[$RANDOM % ${#vals[@]}]}"
}

# --- Generate Config Values ---

# Data TLB
dtlb_sets=$(pow2 1 256)
dtlb_assoc=$(( (RANDOM % 8) + 1 ))

# Page Table (flexible model)
while true; do
    virt_pages=$(pow2 1 8192)
    page_size=$(pow2 1024 4294967296)  # max 4GB
    total_virtual=$((virt_pages * page_size))
    if [ "$total_virtual" -le 4294967296 ]; then
        break
    fi
done


phys_pages=$(pow2 1 1024)

# Data Cache
dc_sets=$(pow2 1 8192)
dc_assoc=$(( (RANDOM % 8) + 1 ))
dc_line=$(pow2 8 1024)  # at least 8
wt_nwa_dc=$([ $((RANDOM % 2)) -eq 0 ] && echo "y" || echo "n")

# L2 Cache
l2_sets=$(pow2 1 8192)
l2_assoc=$(( (RANDOM % 8) + 1 ))
l2_line=$dc_line  # must equal DC line size
wt_nwa_l2=$([ $((RANDOM % 2)) -eq 0 ] && echo "y" || echo "n")

# Features
virt_addr=$([ $((RANDOM % 2)) -eq 0 ] && echo "y" || echo "n")
if [ "$virt_addr" = "n" ]; then
    tlb="n"   # force disabled
else
    tlb=$([ $((RANDOM % 2)) -eq 0 ] && echo "y" || echo "n")
fi
l2=$([ $((RANDOM % 2)) -eq 0 ] && echo "y" || echo "n")

# --- Write Config File ---
cat > trace.config <<EOF
Data TLB configuration
Number of sets: $dtlb_sets
Set size: $dtlb_assoc

Page Table configuration
Number of virtual pages: $virt_pages
Number of physical pages: $phys_pages
Page size: $page_size

Data Cache configuration
Number of sets: $dc_sets
Set size: $dc_assoc
Line size: $dc_line
Write through/no write allocate: n

L2 Cache configuration
Number of sets: $l2_sets
Set size: $l2_assoc
Line size: $l2_line
Write through/no write allocate: n

Virtual addresses: $virt_addr
TLB: n
L2 cache: $l2
EOF

echo "Generated trace.config"


> trace.dat


virt_space=$((virt_pages * page_size))
if [ $virt_space -gt 4294967296 ]; then
    virt_space=4294967296
fi


addr_bits=$(awk -v v=$virt_space 'BEGIN {
    b=0;
    while (2^b < v) b++;
    print b
}')


for j in $(seq 1 5000); do
    op=$([ $((RANDOM % 2)) -eq 0 ] && echo "R" || echo "W")
    raw=$(od -An -N4 -tu4 /dev/urandom | tr -d ' ')
    raw=$(( raw % virt_space ))
    addr=$(printf "%08x" $raw)
    echo "$op:$addr" >> trace.dat
done

echo "Generated trace.dat"

./main < trace.dat > out.txt
./memhier_ref < trace.dat > ref_out.txt
diff -u out.txt ref_out.txt > diff.txt

# Optional: show summary
if [ $? -eq 0 ]; then
    echo "Outputs match. No differences."
    echo ""
else
    echo "Outputs differ. See diff.txt"
fi
