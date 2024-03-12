/****************************************************************************
 * FILE: mpi_heat2D.c
 * OTHER FILES: draw_heat.c
 * DESCRIPTIONS:
 *   HEAT2D Example - Parallelized C Version
 *   This example is based on a simplified two-dimensional heat
 *   equation domain decomposition.  The initial temperature is computed to be
 *   high in the middle of the domain and zero at the boundaries.  The
 *   boundaries are held at zero throughout the simulation.  During the
 *   time-stepping, an array containing two domains is used; these domains
 *   alternate between old data and new data.
 *
 *   In this parallelized version, the grid is decomposed by the master
 *   process and then distributed by rows to the worker processes.  At each
 *   time step, worker processes must exchange border data with neighbors,
 *   because a grid point's current temperature depends upon it's previous
 *   time step value plus the values of the neighboring grid points.  Upon
 *   completion of all time steps, the worker processes return their results
 *   to the master process.
 *
 *   Two data files are produced: an initial data set and a final data set.
 *   An X graphic of these two states displays after all calculations have
 *   completed.
 * AUTHOR: Blaise Barney - adapted from D. Turner's serial C version. Converted
 *   to MPI: George L. Gusciora (1/95)
 * LAST REVISED: 06/12/13 Blaise Barney
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
extern void draw_heat(int nx, int ny);       /* X routine to create graph */

#define NXPROB      20                 /* x dimension of problem grid */
#define NYPROB      20                 /* y dimension of problem grid */
#define STEPS       100                /* number of time steps */
#define MAXWORKER   8                  /* maximum number of worker tasks */
#define MINWORKER   3                  /* minimum number of worker tasks */
#define BEGIN       1                  /* message tag */
#define LTAG        2                  /* message tag */
#define RTAG        3                  /* message tag */
#define NONE        0                  /* indicates no neighbor */
#define DONE        4                  /* message tag */
#define MASTER      0                  /* taskid of first process */

struct Parms {
  float cx;
  float cy;
} parms = {0.1, 0.1};

int main (int argc, char *argv[])
{
void inidat(), prtdat(), update();
float  u[2][NXPROB][NYPROB];        /* array for grid */
float  u2[2][NXPROB][NYPROB];        /* array for grid */
int     taskid,                     /* this task's unique id */
        numworkers,                 /* number of worker processes */
        numtasks,                   /* number of tasks */
        averow,rows,offset,extra,   /* for sending rows of data */
        dest, source,               /* to - from for message send-receive */
        left,right,        /* neighbor tasks */
        msgtype,                    /* for message types */
        rc,start,end,               /* misc */
        i,ix,iy,iz,it;              /* loop variables */

  printf ("Starting mpi_heat2D with %d worker tasks.\n", numworkers);

  /* Initialize grid */
  printf("Grid size: X= %d  Y= %d  Time steps= %d\n",NXPROB,NYPROB,STEPS);
  printf("Initializing grid and writing initial.dat file...\n");
  inidat(NXPROB, NYPROB, u);
  prtdat(NXPROB, NYPROB, u, "initial.dat");

  numworkers = 1;

      averow = NXPROB/numworkers;
      extra = NXPROB%numworkers;
      offset = 0;
      for (i=1; i<=numworkers; i++)
      {
         rows = (i <= extra) ? averow+1 : averow;
         /* Tell each worker who its neighbors are, since they must exchange */
         /* data with each other. */
         if (i == 1)
            left = NONE;
         else
            left = i - 1;
         if (i == numworkers)
            right = NONE;
         else
            right = i + 1;
         /*  Now send startup information to each worker  */
         //MPI_Send(&u[0][offset][0], rows*NYPROB, MPI_FLOAT, dest, BEGIN,
         //         MPI_COMM_WORLD);

      }

      /* Initialize everything - including the borders - to zero */
      for (iz=0; iz<2; iz++)
         for (ix=0; ix<NXPROB; ix++)
            for (iy=0; iy<NYPROB; iy++) {
               u2[iz][ix][iy] = 0.0;
            }
      //MPI_Recv(&u[0][offset][0], rows*NYPROB, MPI_FLOAT, source, msgtype,
      //         MPI_COMM_WORLD, &status);
      memcpy(&u2[0][offset][0], &u[0][offset][0], rows*NYPROB*sizeof(float));
      // Sent to task 2: rows= 7 offset= 7 left= 1 right= 3
      // task=2  start=7  end=13
      // printf("task=%d  start=%d  end=%d\n",taskid,start,end);
      start=offset;
      end=offset+rows-1;
      if (offset==0)
         start=1;
      if ((offset+rows)==NXPROB)
         end--;
      printf("task=%d  start=%d  offset=%d rows=%d end=%d\n",taskid,start,offset, rows,end);
      // main.c task=0  start=1  offset=0 rows=20 end=18
      // raw2.c task=1  start=1  offset=0 rows=20 end=18
      iz = 0;
      for (it = 1; it <= STEPS; it++)
      {
         /* Now call update to update the value of grid points */
         //update(0,NXPROB,NYPROB,&u[iz][0][0],&u[1-iz][0][0]);
         //printf("iz0 %d\n", iz);
         //update(0,NXPROB,NYPROB,&u[iz][0][0],&u[1-iz][0][0]);
         printf("start=%d end=%d iz=%d\n", start, end, iz); fflush(0);
         update(start,end,NYPROB,&u2[iz][0][0],&u2[1-iz][0][0]);
         //printf("iz1 %d\n", iz);
         iz = 1 - iz;
         //printf("iz2 %d\n", iz);
      }


  /* Write final output, call X graph and finalize MPI */
  printf("Writing final.dat file and generating graph...\n");
  prtdat(NXPROB, NYPROB, &u2[0][0][0], "main.final.dat");
  printf("Click on MORE button to view initial/final states.\n");
  printf("Click on EXIT button to quit program.\n");
  return 0;
}


/**************************************************************************
 *  subroutine update
 ****************************************************************************/
void update(int start, int end, int ny, float *u1, float *u2)
{
   int ix, iy;
   for (ix = start; ix <= end; ix++)
      for (iy = 1; iy <= ny-2; iy++)
         *(u2+ix*ny+iy) = *(u1+ix*ny+iy)  +
                          parms.cx * (*(u1+(ix+1)*ny+iy) +
                          *(u1+(ix-1)*ny+iy) -
                          2.0 * *(u1+ix*ny+iy)) +
                          parms.cy * (*(u1+ix*ny+iy+1) +
                         *(u1+ix*ny+iy-1) -
                          2.0 * *(u1+ix*ny+iy));
}

/*****************************************************************************
 *  subroutine inidat
 *****************************************************************************/
void inidat(int nx, int ny, float *u) {
int ix, iy;

for (ix = 0; ix <= nx-1; ix++)
  for (iy = 0; iy <= ny-1; iy++)
     *(u+ix*ny+iy) = (float)(ix * (nx - ix - 1) * iy * (ny - iy - 1));
}

/**************************************************************************
 * subroutine prtdat
 **************************************************************************/
void prtdat(int nx, int ny, float *u1, char *fnam) {
int ix, iy;
FILE *fp;

fp = fopen(fnam, "w");
for (iy = ny-1; iy >= 0; iy--) {
  for (ix = 0; ix <= nx-1; ix++) {
    fprintf(fp, "%8.1f", *(u1+ix*ny+iy));
    if (ix != nx-1)
      fprintf(fp, " ");
    else
      fprintf(fp, "\n");
    }
  }
fclose(fp);
}
