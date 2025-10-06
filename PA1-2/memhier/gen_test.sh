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
    page_size=$(pow2 1024 1048576)  # 1 KB – 1 MB
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

# Ensure L2 line size >= DC line size, both powers of 2
# Get all possible power-of-two values within range [dc_line, 1024]
possible_l2_lines=()
val=$dc_line
while [ $val -le 1024 ]; do
    possible_l2_lines+=($val)
    val=$((val * 2))
done
l2_line=${possible_l2_lines[$RANDOM % ${#possible_l2_lines[@]}]}

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
Write through/no write allocate: $wt_nwa_dc

L2 Cache configuration
Number of sets: $l2_sets
Set size: $l2_assoc
Line size: $l2_line
Write through/no write allocate: $wt_nwa_l2

Virtual addresses: y
TLB: $tlb
L2 cache: y
EOF

echo "Generated trace.config"


> trace.dat


# Compute the maximum addressable space based on virtual or physical addresses
virt_space=$((virt_pages * page_size))
phys_space=$((phys_pages * page_size))

# Cap to 4GB max since addresses are 32-bit
max_space=1073741824
if [ $virt_space -gt $max_space ]; then virt_space=$max_space; fi
if [ $phys_space -gt $max_space ]; then phys_space=$max_space; fi

# Decide which space to use for address generation
if [ "$virt_addr" = "y" ]; then
    addr_limit=$virt_space
else
    addr_limit=$phys_space
fi

# Compute the number of bits needed
addr_bits=$(awk -v v=$addr_limit 'BEGIN {
    b=0;
    while (2^b < v) b++;
    print b
}')

./truncate $addr_bits < long_trace.dat > trace.dat
echo "Truncated trace.dat"
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
