
//  R=[(AxB)+(CxD)]xY
//n_blocks=100
//SIZE_MATRIX : multiple 100
//SIZE_MATRIX 100   1000   10000
//bytes AxB  enviados por iteração : 
//            800   80K    8M 
//nlinhas=SIZE_MATRIX/n_blocks     n_linhas: numero de linhas em cada bloco
#define SIZE_MATRIX 3000
#define n_blocks 100
#include <stdio.h>
#include <unistd.h>
#include <mpi.h>
#include <sys/time.h>
#include "mede_time.h"
int rank;
MPI_Status status;
double R[SIZE_MATRIX][SIZE_MATRIX];

void calculo_R()
{ 
  double X[SIZE_MATRIX][SIZE_MATRIX],Y[SIZE_MATRIX][SIZE_MATRIX];
  double recebido[100][SIZE_MATRIX];
  int i,j,k,l,m;
  int iter,n_linhas;
  n_linhas=SIZE_MATRIX/n_blocks; 
  // R=[(AxB)+(CxD)]xY
  // X = [(AxB)+(CxD)]
  // rank 1 = (AxB)
  // rank 2 = (CxD)
  //inicia matriz X e Y 
  for (i=0;i<SIZE_MATRIX;i++)
      for (j=0;j<SIZE_MATRIX;j++){
            Y[i][j]=i+j;
            X[i][j]=0;
            R[i][j]=0;
        } 
  for (iter=0;iter<n_blocks;iter++){ 
      for(m=0;m<2;m++){
          printf("0 mpi_recv\n"); fflush(0);
            MPI_Recv(
                    &recebido[0][0],
                    SIZE_MATRIX*n_linhas, 
                    MPI_DOUBLE,
                    MPI_ANY_SOURCE,
                    iter,
                    MPI_COMM_WORLD,
                    &status);
          printf("ok mpi_recv\n"); fflush(0);
            for (i=iter*n_linhas;i<iter*n_linhas+n_linhas;i++)
                for(j=0;j<SIZE_MATRIX;j++)
                    X[i][j]=X[i][j]+recebido[i-iter*n_linhas][j];
/*
            if (iter==0) { 
                printf("recebido[0][10]=%f   recebido[5][15]=%f\n",recebido[0][10],recebido[5][15]);
                fflush(stdout);
                printf("recebido[0][0]=%f   recebido[0][10]=%f    recebido[0][99]=%f\n",recebido[0][10],recebido[0][10],recebido[0][99]);
                fflush(stdout);
            }
*/
      }
      // R=[(AxB)+(CxD)]xY
      // X = [(AxB)+(CxD)]
      // rank 1 = (AxB)
      // rank 2 = (CxD)
      for(i=iter*n_linhas;i<iter*n_linhas+n_linhas;i++){  
            for(k=0;k<SIZE_MATRIX;k++)  
                for(j=0;j<SIZE_MATRIX;j++)
                    R[i][j]=R[i][j]+X[i][k]*Y[k][j];
      }
  }
}  

void calculo_AxB()
{
  double A[SIZE_MATRIX][SIZE_MATRIX],B[SIZE_MATRIX][SIZE_MATRIX];
  double result[100][SIZE_MATRIX];
  int i,j,k,l;
  int iter,n_linhas;
  n_linhas=SIZE_MATRIX/n_blocks;
  for (i=0;i<SIZE_MATRIX;i++)
      for (j=0;j<SIZE_MATRIX;j++){
            A[i][j]=2*i+j;
            B[i][j]=i+j+1;
      }
  // R=[(AxB)+(CxD)]xY
  // X = [(AxB)+(CxD)]
  // rank 1 = (AxB)
  // rank 2 = (CxD)
  for (iter=0;iter<n_blocks;iter++){
      for (i=0;i<n_linhas;i++)
          for (j=0;j<SIZE_MATRIX;j++){
                result[i][j]=0;
          }
      for(i=iter*n_linhas;i<iter*n_linhas+n_linhas;i++){
            l=i-iter*n_linhas; 
            for(k=0;k<SIZE_MATRIX;k++)
                for(j=0;j<SIZE_MATRIX;j++)
                    result[l][j]=result[l][j]+A[i][k]*B[k][j];
      }     
      MPI_Send(&result[0][0],SIZE_MATRIX*n_linhas,MPI_DOUBLE,0,iter,MPI_COMM_WORLD);
/*
      if (iter==1){ 
          printf("AxB  result[0][10]=%f  result[5][15]=%f\n",result[0][10],result[5][15]);
          fflush(stdout);
          printf("AxB  result[0][0]=%f  result[0][10]=%f  result[0][99]=%f\n",result[0][0],result[0][10],result[0][99]);
          fflush(stdout);
      }
*/
  }
}
 
void calculo_CxD()
{
  double C[SIZE_MATRIX][SIZE_MATRIX],D[SIZE_MATRIX][SIZE_MATRIX];
  double result[100][SIZE_MATRIX];
  int i,j,k,l;
  int iter,n_linhas;
  n_linhas=SIZE_MATRIX/n_blocks;
  for (i=0;i<SIZE_MATRIX;i++)
      for (j=0;j<SIZE_MATRIX;j++){
            C[i][j]=i+2*j;
            D[i][j]=i+j+2;
      }
  for (iter=0;iter<n_blocks;iter++){
      for (i=0;i<n_linhas;i++)
            for (j=0;j<SIZE_MATRIX;j++){
                result[i][j]=0;
            }
      for(i=iter*n_linhas;i<iter*n_linhas+n_linhas;i++){
            l=i-iter*n_linhas;
            for(k=0;k<SIZE_MATRIX;k++)
                for(j=0;j<SIZE_MATRIX;j++)
                    result[l][j]=result[l][j]+C[i][k]*D[k][j];
      }
      MPI_Send(&result[0][0],SIZE_MATRIX*n_linhas,MPI_DOUBLE,0,iter,MPI_COMM_WORLD);
/*
      if (iter==1){ 
          printf("CxD  result[0][10]=%f   result[5][15]=%f\n",result[0][10],result[5][15]);
          fflush(stdout);
          printf("AxB  result[0][0]=%f  result[0][10]=%f  result[0][99]=%f\n",result[0][0],result[0][10],result[0][99]);
          fflush(stdout);
      }
*/
  }
}
              
int main(int argc, char *argv[])
{
  int n_linhas;
  MPI_Init(&argc,&argv);	
  MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  TIMER_CLEAR;
  TIMER_START;
  n_linhas=SIZE_MATRIX/n_blocks;
  // R=[(AxB)+(CxD)]xY
  if (rank == 0) 
    calculo_R();
  else if (rank==1)
    calculo_AxB();
  else
    calculo_CxD();

  if (rank==0){
      TIMER_STOP;
      printf("TEMPO=%f\n",TIMER_ELAPSED);  
      printf("R[0][0]=%f    R[5][5]=%f R[0][99]=%f,R[15][15]=%f   R[%d][%d]=%f\n",R[0][0],R[5][5],R[0][99],R[15][15],SIZE_MATRIX-1,SIZE_MATRIX-1,R[SIZE_MATRIX-1][SIZE_MATRIX-1]);
      fflush(stdout);
  }
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();
  return 0;
}
