/******************************************************************************
* FILE: mpi_mm.c
* DESCRIPTION:
*   MPI Matrix Multiply - C Version
*   In this code, the master task distributes a matrix multiply
*   operation to numtasks-1 worker tasks.
*   NOTE:  C and Fortran versions of this code differ because of the way
*   arrays are stored/passed.  C arrays are row-major order but Fortran
*   arrays are column-major order.
* AUTHOR: Blaise Barney. Adapted from Ros Leibensperger, Cornell Theory
*   Center. Converted to MPI: George L. Gusciora, MHPCC (1/95)
* LAST REVISED: 04/13/05
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#define NRA 5000                 /* number of rows in matrix A */
#define NCA 5000                 /* number of columns in matrix A */
#define NCB 5000                 /* number of columns in matrix B */
#define MASTER 0               /* taskid of first task */

int main (int argc, char *argv[])
{
int     numtasks,              /* number of tasks in partition */
        taskid,                /* a task identifier */
        numworkers,            /* number of worker tasks */
        source,                /* task id of message source */
        dest,                  /* task id of message destination */
        mtype,                 /* message type */
        rows,                  /* rows of matrix A sent to each worker */
        averow, extra, offset, /* used to determine rows sent to each worker */
        i, j, k, rc;           /* misc */
double  a[NRA][NCA],           /* matrix A to be multiplied */
        b[NCA][NCB],           /* matrix B to be multiplied */
        c[NRA][NCB];           /* result matrix C */



/**************************** master task ************************************/
      printf("mpi_mm has started with %d tasks.\n",numtasks);
      printf("Initializing arrays...\n");
      for (i=0; i<NRA; i++)
         for (j=0; j<NCA; j++)
            a[i][j]= 1;
            //a[i][j]= i+j;
      for (i=0; i<NCA; i++)
         for (j=0; j<NCB; j++)
            b[i][j]= 2;
            //b[i][j]= i*j;

      // run mm
        /**************************** worker task ************************************/
      for (k=0; k<NCB; k++)
         for (i=0; i<NRA; i++)
         {
            c[i][k] = 0.0;
            for (j=0; j<NCA; j++)
               c[i][k] = c[i][k] + a[i][j] * b[j][k];
         }

      return 0;
      /* Print results */
      printf("******************************************************\n");
      printf("Result Matrix:\n");
      for (i=0; i<NRA; i++)
      {
         printf("\n");
         for (j=0; j<NCB; j++)
            printf("%6.2f   ", c[i][j]);
      }
      printf("\n******************************************************\n");
      printf ("Done.\n");

}

