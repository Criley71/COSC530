make
./main < trace.dat > out.txt
./memhier_ref < trace.dat > ref_out.txt
diff -u out.txt ref_out.txt > diff.txt
if [ $? -eq 0 ]; then
    echo "Outputs match. No differences. We ball"
else
    echo "Outputs differ. See diff.txt pain, suffering"
fi