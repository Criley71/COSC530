float dotprod(float x[], float y[], int n) {
  int i;
  float sum;
  sum = 0.0;

  for (i = 0; i < n % 4; i++) {
    sum += x[i] * y[i];
  }
  for (; i < n; i += 4) {
    sum += x[i]*y[i];
    sum += x[i+1]*y[i+1];
    sum += x[i+2]*y[i+2];
    sum += x[i+3]*y[i+3];

  }
  return sum;
}
