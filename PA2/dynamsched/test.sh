while true; do
  ./random_trace
  make
  ./dynamsched < random_trace.dat > ref_out.txt
  ./main < random_trace.dat > out.txt
  diff -u out.txt ref_out.txt > diff.txt
  cat diff.txt
  if [ -f diff.txt ] && [ $(wc -l < diff.txt) -gt 9 ]; then
        echo "diff.txt has more than 12 lines. EXITING :("
        ./dynamsched -v < random_trace.dat > det_ref_out.txt
        diff out.txt ref_out.txt -y --suppress-common-lines
        break
  fi
done