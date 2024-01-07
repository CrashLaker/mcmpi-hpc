#include <mpi.h>
#include <stdio.h>
#include <unistd.h> // for sleep
#include <string.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>

#define VERBOSE 1
//#define VB3(a) if (VERBOSE) { printf("[%lu][client][0] ", (unsigned long)time(NULL)); printf a ; fflush(stdout); }
#define VB3(a) if (VERBOSE) { printf("[%lu][%s][%d] ", (unsigned long)time(NULL), hostname, myrank); printf a ; fflush(stdout); }
// client
pthread_t tid_client_recv;
void client_work();
void client_recv();
//#define WKSIZE 800
//int timestart[WKSIZE];
int *timestart;
MPI_Comm client_comm;
char hostname[MPI_MAX_PROCESSOR_NAME];
int myrank = 0;

int main(int argc, char** argv) {
    // Initialize the MPI environment
    printf("CLIENT START\n"); fflush(0);
    putenv("MCMPI_FORCE_NATIVE=1");
    MPI_Init(NULL, NULL);

    printf("===================== Main =====================\n");
    fflush(0);

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    myrank = world_rank;

    // Get the name of the processor
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);
    MPI_Get_processor_name(hostname, &name_len);

    // Print off a hello world message
    printf("%%Hello world from processor %s, rank %d out of %d processors\n",
           processor_name, world_rank, world_size);
    fflush(stdout);

    //MPI_Comm parent;
    MPI_Comm_get_parent(&client_comm);

    if (client_comm != MPI_COMM_NULL){
        printf("starting the client\n"); fflush(0);
        int a = 2022;
        MPI_Send(&a, 1, MPI_INT, 0, 0, client_comm);
        client_work();
    }


    sleep(1000000);
    MPI_Finalize();

}

void client_work(){
    VB3(("TODO client work\n"));
    if (pthread_create(&tid_client_recv, 
                       NULL, 
                       client_recv, 
                       NULL)){
        printf("tid_client_recv error\n");
    } 
    sleep(2);
    int i;
    MPI_Request treq;
    int sleepm = 550;
    int WKSIZE = 800;
    timestart = (int*) malloc(sizeof(int)*WKSIZE);
    for (i = 0; i < WKSIZE; i++){
        VB3(("To send reqid %d\n", i));
        timestart[i] = (int)time(NULL);
        MPI_Isend(&i,
                  1,
                  MPI_INT,
                  0,
                  50,
                  //mcmpi_comm,
                  client_comm,
                  &treq); 
        usleep(sleepm*1000);
        //if (i == 30) sleepm = 100;
        //if (i == 100) sleepm = 800;
        if (i == 400) sleepm = 400;
        if (i == 450) sleepm = 500;
        //if (i == 300) sleepm = 200;
        //if (i == 1000) sleepm = 400;
    }
    if (pthread_join(tid_client_recv)){
        printf("tid_client_recv join error\n");
    }
}

void client_recv(){
    VB3(("THREAD Service Recv UP\n"));

    int flag, tag, src, idx;
    MPI_Status probestatus;
    while (1){
        flag = 0;
        while (!flag){
            MPI_Iprobe(MPI_ANY_SOURCE,
                       50,
                       client_comm,
                       &flag,
                       &probestatus);
            if (flag) break;
        }
        tag = probestatus.MPI_TAG;
        src = probestatus.MPI_SOURCE;
        int reqid;
        MPI_Recv(&reqid,
                 1,
                 MPI_INT,
                 src,
                 tag,
                 client_comm,
                 MPI_STATUS_IGNORE
                );
        VB3(("service recv received reqid %d\n", reqid));
        int end_ts = (int)time(NULL);
        VB3(("request %d finished in %d seconds\n", reqid, 
                    end_ts - timestart[reqid]));
    }
}
