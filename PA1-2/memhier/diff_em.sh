make
./main < trace.dat > out.txt
./memhier_ref < trace.dat > ref_out.txt
diff -u out.txt ref_out.txt > diff.txt