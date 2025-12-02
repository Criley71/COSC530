float dotprod(float x[], float y[], int n)
{
  int i;
  float sum0 = 0.0;
  float sum1 = 0.0;
  float sum2 = 0.0; 
  float sum3 = 0.0; 
  float sum4 = 0.0; 

  for(i = 0; i < n%4; i++){
    sum0 += x[i]*y[i];
  }
  for(; i< n; i+=4){
    sum1 += x[i]*y[i];
    sum2 += x[i+1]*y[i+1];
    sum3 += x[i+2]*y[i+2];
    sum4 += x[i+3]*y[i+3];
  }
  return (sum0+sum1+sum2+sum3+sum4);
}
