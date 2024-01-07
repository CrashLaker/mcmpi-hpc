
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

double begin, b0, b1, b2, b3;
double end, e0, e1, e2, e3;

#define st MPI_Wtime()


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
            MPI_Recv(
                    &recebido[0][0],
                    SIZE_MATRIX*n_linhas, 
                    MPI_DOUBLE,
                    MPI_ANY_SOURCE,
                    iter,
                    MPI_COMM_WORLD,
                    &status);
            for (i=iter*n_linhas;i<iter*n_linhas+n_linhas;i++)
                for(j=0;j<SIZE_MATRIX;j++)
                    X[i][j]=X[i][j]+recebido[i-iter*n_linhas][j];
      }
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
  }
}
              
int main(int argc, char *argv[])
{
  int n_linhas;
  printf("Start\n");fflush(0);
  MPI_Init(&argc,&argv);	
  MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  TIMER_CLEAR;
  TIMER_START;
  n_linhas=SIZE_MATRIX/n_blocks;
  // R=[(AxB)+(CxD)]xY
  if (rank == 0) {
    printf("Start _R\n");fflush(0);
    calculo_R();
  }else if (rank==1){
    printf("Start AxB\n");fflush(0);
    calculo_AxB();
  }else{
    printf("Start CxD\n");fflush(0);
    calculo_CxD();
  }

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
