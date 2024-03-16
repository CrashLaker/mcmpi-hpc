/******************************************************************************
* FILE: ser_mm.c
* DESCRIPTION:  
*   Serial Matrix Multiply - C Version
* AUTHOR: Blaise Barney
* LAST REVISED: 04/12/05
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include <sys/times.h>
struct timeval tv1, tv2;
#define TIMER_CLEAR   (tv1.tv_sec = tv1.tv_usec = tv2.tv_sec = tv2.tv_usec=0)
#define TIMER_START   gettimeofday(&tv1, (struct timezone*)0)
#define TIMER_STOP    gettimeofday(&tv2, (struct timezone*)0)
#define TIMER_ELAPSED (tv2.tv_sec-tv1.tv_sec+(tv2.tv_usec-tv1.tv_usec)*1.E-6)
struct timeval tv_fsend1, tv_fsend2;
#define TIMER_CLEAR_S   (tv_fsend1.tv_sec = tv_fsend1.tv_usec = tv_fsend2.tv_sec = tv_fsend2.tv_usec=0)
#define TIMER_START_S   gettimeofday(&tv_fsend1, (struct timezone*)0)
#define TIMER_STOP_S    gettimeofday(&tv_fsend2, (struct timezone*)0)
#define TIMER_ELAPSED_S (tv_fsend2.tv_sec-tv_fsend1.tv_sec+(tv_fsend2.tv_usec-tv_fsend1.tv_usec)*1.E-6)
struct timeval tv_frecv1, tv_frecv2;
#define TIMER_CLEAR_R   (tv_frecv1.tv_sec = tv_frecv1.tv_usec = tv_frecv2.tv_sec = tv_frecv2.tv_usec=0)
#define TIMER_START_R   gettimeofday(&tv_frecv1, (struct timezone*)0)
#define TIMER_STOP_R    gettimeofday(&tv_frecv2, (struct timezone*)0)
#define TIMER_ELAPSED_R (tv_frecv2.tv_sec-tv_frecv1.tv_sec+(tv_frecv2.tv_usec-tv_frecv1.tv_usec)*1.E-6)
int SIZE;

#define NRA 5000 			/* number of rows in matrix A */
#define NCA 5000			/* number of columns in matrix A */
#define NCB 5000   		/* number of columns in matrix B */

int main(int argc, char *argv[])
{
int    i, j, k;			/* misc */
double a[NRA][NCA], 		/* matrix A to be multiplied */
       b[NCA][NCB],      	/* matrix B to be multiplied */
       c[NRA][NCB];		/* result matrix C */

TIMER_CLEAR;
TIMER_START;
printf("Starting serial matrix multiple example...\n");
printf("Using matrix sizes a[%d][%d], b[%d][%d], c[%d][%d]\n",
        NRA,NCA,NCA,NCB,NRA,NCB);

/* Initialize A, B, and C matrices */
printf("Initializing matrices...\n");
for (i=0; i<NRA; i++)
   for (j=0; j<NCA; j++)
      a[i][j]= i+j;
for (i=0; i<NCA; i++)
   for (j=0; j<NCB; j++)
      b[i][j]= i*j;
for(i=0;i<NRA;i++)
   for(j=0;j<NCB;j++)
      c[i][j] = 0.0;

/* Perform matrix multiply */
printf("Performing matrix multiply...\n");
for(i=0;i<NRA;i++)
   for(j=0;j<NCB;j++)
      for(k=0;k<NCA;k++)
         c[i][j]+= a[i][k] * b[k][j];
TIMER_STOP;
printf ("TEMPO: %12.7f\n",TIMER_ELAPSED );
return 0;
printf("Here is the result matrix:");
for (i=0; i<NRA; i++) { 
   printf("\n"); 
   for (j=0; j<NCB; j++) 
      printf("%6.2f   ", c[i][j]);
   }
printf ("\nDone.\n");
}
