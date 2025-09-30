awk 'NR==FNR { a[++n]=$0; next }
     { b[++m]=$0 }
     END {
       i=1; j=1;
       while (i<=n || j<=m) {
         for (k=0; k<2 && i<=n; k++) print a[i++];
         for (k=0; k<5 && j<=m; k++) print b[j++];
       }
     }' trace_out.txt trace_ref.txt > combined.txt
