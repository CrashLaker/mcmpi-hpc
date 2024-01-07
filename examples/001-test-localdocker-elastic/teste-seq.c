
//  R=[(AxB)+(CxD)]xY
//n_blocks=100
//SIZE_MATRIX : multiple 100
//SIZE_MATRIX 100   1000   10000
//bytes AxB  enviados por iteração : 
//            800   80K    8M 
//nlinhas=SIZE_MATRIX/n_blocks     n_linhas: numero de linhas em cada bloco
#define SIZE_MATRIX 1000
#define n_blocks 100
#include <stdio.h>
#include <unistd.h>
#include <mpi.h>
#include <sys/time.h>
#include "mede_time.h"
int rank;
MPI_Status status;
double R[SIZE_MATRIX][SIZE_MATRIX];
double R1[SIZE_MATRIX][SIZE_MATRIX];
double R2[SIZE_MATRIX][SIZE_MATRIX];

void calculo_R()
{ 
  double X[SIZE_MATRIX][SIZE_MATRIX],Y[SIZE_MATRIX][SIZE_MATRIX];
  double recebido[SIZE_MATRIX][100];
  int i,j,k;
  //inicia matriz X e Y 
  for (i=0;i<SIZE_MATRIX;i++)
      for (j=0;j<SIZE_MATRIX;j++){
            Y[i][j]=i+j;
            //Y[i][j]=1;
            R[i][j]=0;
        } 
      for(i=0;i<SIZE_MATRIX;i++)  
            for(k=0;k<SIZE_MATRIX;k++)  
                for(j=0;j<SIZE_MATRIX;j++)
                    R[i][j]=R[i][j]+(R1[i][k]+R2[i][k])*Y[k][j];
}  

void calculo_AxB()
{
  double A[SIZE_MATRIX][SIZE_MATRIX],B[SIZE_MATRIX][SIZE_MATRIX];
  int i,j,k;
  for (i=0;i<SIZE_MATRIX;i++)
      for (j=0;j<SIZE_MATRIX;j++){
            A[i][j]=2*i+j;
            //A[i][j]=1;
            B[i][j]=i+j+1;
            //B[i][j]=1;
            R1[i][j]=0;
      }
  for(i=0;i<SIZE_MATRIX;i++){
            for(k=0;k<SIZE_MATRIX;k++)
                for(j=0;j<SIZE_MATRIX;j++)
                    R1[i][j]=R1[i][j]+A[i][k]*B[k][j];
      }     
}
 
void calculo_CxD()
{
  double C[SIZE_MATRIX][SIZE_MATRIX],D[SIZE_MATRIX][SIZE_MATRIX];
  int i,j,k;
  for (i=0;i<SIZE_MATRIX;i++)
      for (j=0;j<SIZE_MATRIX;j++){
            C[i][j]=i+2*j;
            D[i][j]=i+j+2;
            //C[i][j]=1;
            //D[i][j]=1;
            R2[i][j]=0;
      }
  for(i=0;i<SIZE_MATRIX;i++){
            for(k=0;k<SIZE_MATRIX;k++)
                for(j=0;j<SIZE_MATRIX;j++)
                    R2[i][j]=R2[i][j]+C[i][k]*D[k][j];
      }     

}
              
int main(int argc, char *argv[])
{
  TIMER_CLEAR;
  TIMER_START;
  calculo_AxB();
  calculo_CxD();
  calculo_R();
  TIMER_STOP;
  printf("TEMPO=%f\n",TIMER_ELAPSED); 
//  printf("R1[0][0]=%f    R2[0][0]=%f\n",R1[0][0],R2[0][0]); 
//  printf("R1[0][10]=%f    R2[0][10]=%f\n",R1[0][10],R2[0][10]); 
//  printf("R1[0][99]=%f    R2[0][99]=%f\n",R1[0][99],R2[0][99]); 
//  printf("R1[5][5]=%f    R2[5][5]=%f\n",R1[5][5],R2[5][5]); 
//  printf("R1[10][10]=%f    R2[10][10]=%f\n",R1[10][10],R2[10][10]); 
//  printf("R1[15][15]=%f    R2[15][15]=%f\n",R1[15][15],R2[15][15]); 
  printf("R[0][0]=%f    R[5][5]=%f R[0][99]=%f  R[15][15]=%f   R[%d][%d]=%f\n",R[0][0],R[5][5],R[0][99],R[15][15],SIZE_MATRIX-1,SIZE_MATRIX-1,R[SIZE_MATRIX-1][SIZE_MATRIX-1]);
  fflush(stdout);
  return 0;
}
