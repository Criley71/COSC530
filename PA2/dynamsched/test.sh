counter=0
while [ $counter -lt 10000 ]; do
  ./random_trace
  make
  ./dynamsched < random_trace.dat > ref_out.txt
  ./main < random_trace.dat > out.txt
  diff -u out.txt ref_out.txt > diff.txt
  cat diff.txt
  counter=$((counter+1))
  if [ -f diff.txt ] && [ $(wc -l < diff.txt) -gt 9 ]; then
        echo "diff.txt has more than 12 lines. EXITING :("
        ./dynamsched -v < random_trace.dat > det_ref_out.txt
        diff out.txt ref_out.txt -y --suppress-common-lines
        break
  fi
done
if [ $counter == 10000 ]; then
  echo "it made it through 10000"
fi