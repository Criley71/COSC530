#!/bin/bash

# Parse numeric values from trace.config
virt_pages=$(grep "Number of virtual pages:" trace.config | awk '{print $5}')
phys_pages=$(grep "Number of physical pages:" trace.config | awk '{print $5}')
page_size=$(grep "Page size:" trace.config | awk '{print $3}')
virt_addr=$(grep "Virtual addresses:" trace.config | awk '{print $3}')

# Compute log2 for each (using awk math)
log2_virt=$(awk -v x="$virt_pages" 'BEGIN{print log(x)/log(2)}')
log2_phys=$(awk -v x="$phys_pages" 'BEGIN{print log(x)/log(2)}')
log2_page=$(awk -v x="$page_size"  'BEGIN{print log(x)/log(2)}')

# Decide which total to use
if [[ "$virt_addr" == "y" ]]; then
    total_bits=$(awk -v a="$log2_virt" -v b="$log2_page" 'BEGIN{print int(a+b)}')
else
    total_bits=$(awk -v a="$log2_phys" -v b="$log2_page" 'BEGIN{print int(a+b)}')
fi

#echo "Total address bits: $total_bits"
./truncate $total_bits < long_trace.dat > trace.dat
make
echo "truncated"
./main < trace.dat > out.txt
echo "main done"
./memhier_ref < trace.dat > ref_out.txt
echo "ref done"
diff -u out.txt ref_out.txt > diff.txt

if grep -q "memory" diff.txt || [ $(wc -l < diff.txt) -gt 12 ]; then
    echo "Outputs differ. See diff.txt pain, suffering"
else
    echo "Outputs match. No differences. We ball"
fi
