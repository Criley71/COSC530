float dotprod(float x[], float y[], int n)
{
   int i;
   float sum;

   sum = 0.0;
   for (i = 0; i < n; i++)
      sum += x[i] * y[i];
   return sum;
}

