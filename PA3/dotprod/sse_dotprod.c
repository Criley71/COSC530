#include <emmintrin.h>
float dotprod(float x[], float y[], int n) {
  float *m1 __attribute__((aligned(16)));
  float *m2 __attribute__((aligned(16)));

  __m128 mx = {0.0, 0.0, 0.0, 0.0}; // x values
  __m128 my = {0.0, 0.0, 0.0, 0.0}; // y values
  __m128 mm = {0.0, 0.0, 0.0, 0.0}; // mult matrix
  
  float sum = 0.0;
  int i;
  
  for (i = 0; i < n; i += 4) {
    __m128 sm = {0.0, 0.0, 0.0, 0.0}; // sum matrix
    m1 = x+i;
    m2 = y+i;

    mx = _mm_load_ps(m1);
    my = _mm_load_ps(m2);

    mm = _mm_mul_ps (mx, my); //multiply the 2 vectors
    sm = _mm_add_ps (mm, sm); //add the new product to the sum

    float temp_sum[4] __attribute__ ((aligned(16))) = {0.0, 0.0, 0.0, 0.0};//temp array for sums
    _mm_store_ps (temp_sum, sm);
    sum += temp_sum[0]+temp_sum[1]+temp_sum[2]+temp_sum[3];
  }
  return sum;
  }
