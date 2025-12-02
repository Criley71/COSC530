#include <sys/types.h>
#include <sys/times.h>
#include <stdlib.h>
#include <float.h>
#include <stdio.h>

/*
 * ttime - Returns in milliseconds the time prior to it being called.
 */
int ttime()
{
   struct tms buffer;
   int utime;

   times(&buffer);
   utime = (buffer.tms_utime / 60.0) * 1000.0;
   return (utime);
}

void init(float a[], float b[])
{
   int i;

   for (i = 0; i < SIZE; i++) {
      a[i] = 1;
      b[i] = 1;
   }
}

int main()
{
   float totaltime, tmptime, starttime, endtime, result;
   int i;
   float dotprod(float [], float [], int);
   __attribute__ ((aligned(16))) float a[SIZE], b[SIZE];

#define NUM 100000

   init(a, b);
   starttime = ttime();
   for (i = 0; i < NUM; i++)
      result = dotprod(a, b, SIZE);
   endtime = ttime();
   totaltime = (endtime - starttime);
   printf("result is %7.1f\n", result);
   printf("average dotprod time is %5.5f milliseconds\n", totaltime/NUM);
   exit(0);
}
