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
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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

#define NRA 5000                 /* number of rows in matrix A */
#define NCA 5000                 /* number of columns in matrix A */
#define NCB 5000                 /* number of columns in matrix B */
#define MASTER 0               /* taskid of first task */
#define FROM_MASTER 1          /* setting a message type */
#define FROM_WORKER 2          /* setting a message type */

int main (int argc, char *argv[])
{
int	numtasks,              /* number of tasks in partition */
	taskid,                /* a task identifier */
	numworkers,            /* number of worker tasks */
	source,                /* task id of message source */
	dest,                  /* task id of message destination */
	mtype,                 /* message type */
	rows,                  /* rows of matrix A sent to each worker */
	averow, extra, offset, /* used to determine rows sent to each worker */
	i, j, k, rc;           /* misc */
double	a[NRA][NCA],           /* matrix A to be multiplied */
	b[NCA][NCB],           /* matrix B to be multiplied */
	c[NRA][NCB];           /* result matrix C */
MPI_Status status;

MPI_Init(&argc,&argv);


MPI_Comm_rank(MPI_COMM_WORLD,&taskid);
MPI_Comm_size(MPI_COMM_WORLD,&numtasks);
int name_len;
char hostname[1000];
MPI_Get_processor_name(hostname, &name_len);

printf("Hello world from processor %s, rank %d out of %d processors\n",
           hostname, taskid, numtasks);
fflush(0);
if (numtasks < 2 ) {
  printf("Need at least two MPI tasks. Quitting...\n");
  MPI_Abort(MPI_COMM_WORLD, rc);
  exit(1);
  }
numworkers = numtasks-1;

printf("=============== START ===============\n\n"); fflush(0);

/**************************** master task ************************************/
   if (taskid == MASTER)
   {
      TIMER_CLEAR;
      TIMER_CLEAR_S;
      TIMER_CLEAR_R;
      TIMER_START;
      printf("mpi_mm has started with %d tasks.\n",numtasks);
      //printf("mpi_mm has started with %d tasks.\n",numtasks); fflush(stdout);
      printf("Initializing arrays...\n"); 
      //printf("Initializing arrays...\n"); fflush(stdout);
      for (i=0; i<NRA; i++)
         for (j=0; j<NCA; j++)
            a[i][j]= i+j;
      for (i=0; i<NCA; i++)
         for (j=0; j<NCB; j++)
            b[i][j]= i*j;

      /* Send matrix data to the worker tasks */
      averow = NRA/numworkers;
      extra = NRA%numworkers;
      offset = 0;
      mtype = FROM_MASTER;
      TIMER_START_S;
      for (dest=1; dest<=numworkers; dest++)
      {
         rows = (dest <= extra) ? averow+1 : averow;   	
         printf("Sending %d rows to task %d offset=%d\n",rows,dest,offset); 
         //printf("Sending %d rows to task %d offset=%d\n",rows,dest,offset); fflush(stdout);
         MPI_Send(&offset, 1, MPI_INT, dest, mtype, MPI_COMM_WORLD);
         MPI_Send(&rows, 1, MPI_INT, dest, mtype, MPI_COMM_WORLD);
         MPI_Send(&a[offset][0], rows*NCA, MPI_DOUBLE, dest, mtype,
                   MPI_COMM_WORLD);
         MPI_Send(&b, NCA*NCB, MPI_DOUBLE, dest, mtype, MPI_COMM_WORLD);
         offset = offset + rows;
      }
      TIMER_STOP_S;
      printf ("TEMPO for send [%d]: %12.7f\n",SIZE,TIMER_ELAPSED_S );

      /* Receive results from worker tasks */
      mtype = FROM_WORKER;
      TIMER_START_R;
      for (i=1; i<=numworkers; i++)
      {
         source = i;
         MPI_Recv(&offset, 1, MPI_INT, source, mtype, MPI_COMM_WORLD, &status);
         MPI_Recv(&rows, 1, MPI_INT, source, mtype, MPI_COMM_WORLD, &status);
         MPI_Recv(&c[offset][0], rows*NCB, MPI_DOUBLE, source, mtype, 
                  MPI_COMM_WORLD, &status);
         printf("Received results from task %d\n",source);
         //printf("Received results from task %d\n",source); fflush(stdout);
      }
      TIMER_STOP_R;
      printf ("TEMPO for recv [%d]: %12.7f\n",SIZE,TIMER_ELAPSED_R );

      /* Print results */
      //printf("******************************************************\n");
      //printf("Result Matrix:\n");
      //for (i=0; i<NRA; i++)
      //{
      //   printf("\n"); 
      //   for (j=0; j<NCB; j++) 
      //      printf("%6.2f   ", c[i][j]);
      //}
      //printf("\n******************************************************\n");
      //printf ("Done.\n");
      TIMER_STOP;
      printf ("TEMPO [%d]: %12.7f\n",SIZE,TIMER_ELAPSED );
      //printf ("TEMPO [%d]: %12.7f\n",SIZE,TIMER_ELAPSED ); fflush(stdout);
   }


/**************************** worker task ************************************/
   if (taskid > MASTER)
   {
      mtype = FROM_MASTER;
      MPI_Recv(&offset, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD, &status);
      MPI_Recv(&rows, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD, &status);
      MPI_Recv(&a, rows*NCA, MPI_DOUBLE, MASTER, mtype, MPI_COMM_WORLD, &status);
      MPI_Recv(&b, NCA*NCB, MPI_DOUBLE, MASTER, mtype, MPI_COMM_WORLD, &status);

      for (k=0; k<NCB; k++)
         for (i=0; i<rows; i++)
         {
            c[i][k] = 0.0;
            for (j=0; j<NCA; j++)
               c[i][k] = c[i][k] + a[i][j] * b[j][k];
         }
      mtype = FROM_WORKER;
      MPI_Send(&offset, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD);
      MPI_Send(&rows, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD);
      MPI_Send(&c, rows*NCB, MPI_DOUBLE, MASTER, mtype, MPI_COMM_WORLD);
   }
   //MPI_Barrier(MPI_Barrier);
   sleep(5000);
   MPI_Finalize();
}
