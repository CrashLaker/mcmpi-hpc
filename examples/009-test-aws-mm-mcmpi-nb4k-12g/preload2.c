#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <dlfcn.h>
#include <unistd.h> // for sleep
#include <string.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <math.h>



#define VERBOSE 1
#define VB(a) if (VERBOSE) { printf("[%lu][%s][%d] ", (unsigned long)time(NULL), hostname, myrank); printf a ; fflush(stdout); }
#define VB2(a) if (VERBOSE) { printf("[%lu][%s][%d] ", (unsigned long)time(NULL), hostname, myrank); printf a ; fflush(stdout); }
#define VB3(a) if (VERBOSE) { printf("[%lu][%s][%d] ", (unsigned long)time(NULL), hostname, myrank); printf a ; fflush(stdout); }
#include <mcmpidev.h>

///////////////buffer////////////////////
#include <sys/types.h>
#include <sys/wait.h>
#define DEBUG 0
#define DBUFFER 1
#define DPROBE 1

#define size_buffer 5000
int * buffer1;     //shared
sem_t *excl1;       //shared
sem_t *full1;       //shared
sem_t *empty1;      //shared
int *pointer1;      //shared
/* posicao do prox. item a ser lido e removido */
int *count1;

int * buffer2;     //shared
sem_t *excl2;       //shared
sem_t *full2;       //shared
sem_t *empty2;      //shared
int *pointer2;      //shared
/* posicao do prox. item a ser lido e removido */
int *count2;
struct shmid_ds *buf; // del

struct Nodes * cltnodes; // cluster only

sem_t * cluster_barrier;
sem_t * cluster_init;
pthread_mutex_t * send_command_arg_lock;
int barrier_progress = 0;
///////////////////////////////////

//////////////////MPI PRELOAD////////////////////
typedef int (*orig_open_f_type)(const char *pathname, int flags);
typedef int (*mpi_init_f_type)(int *argc, char ***argv);
typedef int (*mpi_send_f_type)(const void *buf, int count, MPI_Datatype type, int dest,
        int tag, MPI_Comm comm);
typedef int (*mpi_recv_f_type)(void *buf, int count, MPI_Datatype type, int source, int tag, MPI_Comm comm, MPI_Status *status);
typedef int (*mpi_comm_size_f_type)(MPI_Comm comm, int *size);
typedef int (*mpi_comm_rank_f_type)(MPI_Comm comm, int *rank);
typedef int (*mpi_comm_rank_f_type)(MPI_Comm comm, int *rank);
typedef int (*mpi_iprobe_f_type)(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status);
typedef int (*mpi_barrier_f_type)(MPI_Comm comm);
typedef int (*mpi_finalize_f_type)(void);
/////////////////////////////////////////////////


//////////////////MPI PRELOAD////////////////////
#define MAX_HOSTNAME_LEN 200
#define MAX_COMMS 200
char hostname[100];
int myrank;
int mysize;
char myhostname;
int mcmpi_add_node(char * server);
int* mcmpi_add_cluster(char * server, char * node_list);
int _mcmpi_add_node(char * server);
void children_stuff();
void *thread_work(void *arg); // thread work for thread join
char ** split_servers(char *line, int * count);
pthread_t tid;
//children
MPI_Info info, minfo;
MPI_Comm parentcomm, wrld, intercomm; 
MPI_Comm clientcomm;
MPI_Comm mycomms[MAX_COMMS];
char parenthostname[1000];
char parentportname[1000];
int rank, size, err;
/////////////////////////////////////////////////
MPI_Status status;


struct NodeTask * head_task;
struct NodeTask * tail_task;

static int a = 2;
static int global_rank = -1;
static int global_size = -1;
int mcmpi_first_message = 0;
static MPI_Comm mcmpi_comm = NULL;
struct Nodes nodes;
struct Commands cmds;

int mcmpi_app = 0;
int mcmpi_cluster = 0;
int mcmpi_cluster_gateway = 0; // 0->no, 1->global, 2->cluster
int grank_start = -1;
int init = 1; // mcmpi_cluster_gateway to wait on mpi_barrier behaviour
int skip_route = 1;
pthread_t pid_inout[2];
int clt_barrier_ongoing_size = 0; // for cluster barrier gateway2 only
sem_t clt_barrier_lock;
sem_t task_loop_lock;
int wait_exit_prologue = 0;
sem_t sem_wait_exit_prologue;

char processor_name[MPI_MAX_PROCESSOR_NAME];
int name_len;
char * mcmpi_app_env;
char * mcmpi_cluster_env2;
char * mcmpi_hostfile_spawned_env; // for nodes spawned by reading mcmpi_hostfile
                                   // when run dynamically
int mcmpi_hostfile_spawned = 1;

char mcmpi_hostfile_flag[200] = "";
char port_name[1000];


int MPI_Init(int *argc, char ***argv)
{

    // start remove rank semaphores
    remove_rank_sem_init();
    // end start remove rank semaphores
    sem_init(&sem_wait_exit_prologue,0,1);  // valor 1
    sem_wait(&sem_wait_exit_prologue);

    VB(("Start MCMPI MPI_Init\n"));

    char * mcmpi_master = getenv("MCMPI_MASTER");

    if (getenv("MCMPI_HOSTFILE_SPAWNED_DISABLE")){
        mcmpi_hostfile_spawned = 0;
    }

    if (NULL != (mcmpi_app_env = getenv("MCMPI_APP"))) {
        mcmpi_app = 1;
    }

    if (NULL != (mcmpi_cluster_env2 = getenv("MCMPI_CLUSTER"))) {
        mcmpi_cluster = 1;
    }

    if (DEBUG) printf("MPI_Init start calling original first\n");
    mpi_init_f_type mpi_init;
    mpi_init = (mpi_init_f_type)dlsym(RTLD_NEXT,"MPI_Init");
    int provided;
    int rs = MPI_Init_thread(argc,argv,MPI_THREAD_MULTIPLE,&provided);
    if (getenv("MCMPI_FORCE_NATIVE")){
        printf("MPI_INIT return MCMPI_FORCE_NATIVE\n"); fflush(0);
        return rs;
    }

    int len;
    MPI_Get_processor_name(hostname, &len);
    MPI_Get_processor_name(processor_name, &len);
    if (DEBUG) printf("MCMPI check will start %d %d\n", mcmpi_app, mcmpi_cluster);

    MPI_Comm_get_parent( &parentcomm );
    if (parentcomm){
        //VB(("child up!\n"));
    }
    int rank, size;
    if (mcmpi_master || mcmpi_cluster){
        // Start MPI_Spawn calls
        // isremoving = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
        // pthread_mutex_init(isremoving, NULL);
        // now that we have remove ranks
        // whenever we use any MPI_Comm
        // if we happen to update that when any MPI function
        // that is about to use that communication
        // that yield an error. so we need to hold every function

        char * mcmpi_hostfile;
        if (NULL != (mcmpi_hostfile = getenv("MCMPI_HOSTFILE")) && mcmpi_cluster != 1) {
            strcpy(nodes.hostname[0], hostname);
            strcpy(nodes.comm[0], "global");
            nodes.type[0] = 0;
            nodes.local_rank[0] = 0;
            nodes.global_rank[0] = 0;
            nodes.size = 1;
            nodes.grank_size = 1; // rank 0 already taken (controller)
            read_mcmpi_file(mcmpi_hostfile);
            dump_commands(&cmds);
            dump_nodes(&nodes);
        }else if (mcmpi_cluster){

            // __  __  ____ __  __ ____ ___        ____
            //|  \/  |/ ___|  \/  |  _ \_ _|      |___ \
            //| |\/| | |   | |\/| | |_) | |         __) |
            //| |  | | |___| |  | |  __/| | ______ / __/
            //|_|  |_|\____|_|  |_|_|  |___|______|_____|
            //                               |__|
            // MPI Gateway Cluster
            VB(("MCMPI_2\n"));
            int world_size;
            int i;
            int node_rank;
            sem_post(&clt_barrier_lock); // start enabled
            sem_post(&task_loop_lock); 
            pthread_create(&tid_task_loop_worker, 
                           NULL, 
                           task_loop_worker_thread, 
                           NULL);
            MPI_Comm_dup(MPI_COMM_WORLD, &mcmpi_comm);
            MPI_Comm_rank(mcmpi_comm, &node_rank);
            MPI_Get_processor_name(processor_name, &name_len);
            MPI_Comm_size(mcmpi_comm, &world_size);
            mysize = world_size;
            if (pthread_create(&tid, NULL, thread_work, NULL)){
                VB(("Cannot create thread\n"));
            }
            if (node_rank == 0){

                //   ____    _  _____ _______        ___ __   __  ____
                //  / ___|  / \|_   _| ____\ \      / / \\ \ / / |___ \
                // | |  _  / _ \ | | |  _|  \ \ /\ / / _ \\ V /    __) |
                // | |_| |/ ___ \| | | |___  \ V  V / ___ \| |    / __/
                //  \____/_/   \_\_| |_____|  \_/\_/_/   \_\_|___|_____|
                //                                          |_____|
                init = 1;
                init_gateway();
                cluster_barrier = (sem_t*) malloc(sizeof(sem_t));
                cluster_init = (sem_t*) malloc(sizeof(sem_t));
                mcmpi_cluster_gateway = 2;
                sem_init(cluster_barrier,1,0);
                sem_init(cluster_init,1,0);
                create_comm_buffers_from_env();
                int tid[2] = {0,1};
                for (i = 0; i < 2; i++)
                    if (pthread_create(&pid_inout[i], NULL, inout_thread, &tid[i])){
                        if (DEBUG) printf("error creating thread\n");
                        if (DEBUG) fflush(stdout);
                    }


                read_mcmpi_file("mcmpi_hostfile");
                char hostnamerec[200];
                while (grank_start == -1){
                    sleep(1);
                }

                int granks = grank_start;
                for (i = 1; i < world_size; i++){
                    MPI_Recv(&hostnamerec, 200, MPI_CHAR, i, 0, mcmpi_comm, MPI_STATUS_IGNORE);
                    MPI_Send(&granks, 1, MPI_INT, i, 0, mcmpi_comm);
                    add_node(hostnamerec, 0, i, granks, processor_name);
                    granks++;
                }
                dump_nodes(&nodes);
                memcpy(cltnodes, &nodes, sizeof(nodes));
                send_command_arg(20, 0, 0, 0, 0, 0, 0);
                sleep(1);
                for (i = 0; i < 2; i++)
                    if (pthread_join(pid_inout[i], NULL)){
                        if (DEBUG) printf("error joining thread\n");
                    }
                MPI_Finalize();
                exit(0);
            }else{
                MPI_Send(&processor_name, name_len+1, MPI_CHAR, 0, 0, mcmpi_comm);
                MPI_Recv(&global_rank, 1, MPI_INT, 0, 0, mcmpi_comm, MPI_STATUS_IGNORE);
            }

            //  ____ _    _   _ ____ _____ _____ ____      _   _  ___  ____  _____
            // / ___| |  | | | / ___|_   _| ____|  _ \    | \ | |/ _ \|  _ \| ____|
            //| |   | |  | | | \___ \ | | |  _| | |_) |   |  \| | | | | | | |  _|
            //| |___| |__| |_| |___) || | | |___|  _ <    | |\  | |_| | |_| | |___
            // \____|_____\___/|____/ |_| |_____|_| \_\___|_| \_|\___/|____/|_____|
            //                                       |_____|
            int tbuff[1000];
            MPI_Recv(&tbuff, 1000, MPI_INT, 0, 7, mcmpi_comm, MPI_STATUS_IGNORE);
            init = 0;
            skip_route = 0;
            mcmpi_first_message = 0;
            MPI_XBarrier(mcmpi_comm);
            return rs;
        }else{
            printf("MPI_INIT return ENV NOT FOUND\n"); fflush(0);
            return rs;
        }
//
            //  ____ ___  _   _ _____ ____   ___  _     _     _____ ____
            // / ___/ _ \| \ | |_   _|  _ \ / _ \| |   | |   | ____|  _ \
            //| |  | | | |  \| | | | | |_) | | | | |   | |   |  _| | |_) |
            //| |__| |_| | |\  | | | |  _ <| |_| | |___| |___| |___|  _ <
            // \____\___/|_| \_| |_| |_| \_\\___/|_____|_____|_____|_| \_\

        global_rank = 0;
        mcmpi_app = 1;
        int server_count = 0;
        dump_nodes(&nodes);
        int i, y;
        for (i = 0; i < cmds.size; i++){
            printf("+++ to provision [%s] = %s\n", cmds.key[i], cmds.list[i]);
            if (strcmp(cmds.key[i],  "global") == 0){
                char **servers_list = split_servers(cmds.list[i], &server_count);
                for (y = 0; y < server_count; y++){
                    mcmpi_add_node(servers_list[y]); 
                    VB(("after mcmpi_add_node\n"));
                    dump_nodes(&nodes);
                }
            }else{ // cluster
                mcmpi_add_cluster(cmds.key[i], cmds.list[i]);
            }
        }
        sync_table();
        sleep(5); // wait first sync_table
        int tbuff[1000];
        // this block will notify all ranks that controller has finished
        // reading mcmpi_hostfile and will also assure that
        // all ranks have the latest sync_table version
        for (i = 1; i < nodes.size; i++){
            if (strcmp(nodes.comm[i], "global") == 0){
                MPI_NSend(
                    &tbuff,
                    1000,
                    MPI_INT,
                    nodes.local_rank[i],
                    7,
                    mcmpi_comm);
            }
        }
        init = 0;
        skip_route = 0;
        MPI_XBarrier(mcmpi_comm);
        strcpy(mcmpi_hostfile_flag, "export MCMPI_HOSTFILE_SPAWNED_DISABLE=1;");
        dump_nodes2(&nodes);

        return rs;
    }else{ // mcmpi_master == NULL
        if (!mcmpi_app) {
            printf("MPI_INIT return !mcmpi_app\n"); fflush(0);
            return rs;
        }
        int rank;
        //  ____  ____   ___        ___   _
        // / ___||  _ \ / \ \      / / \ | |
        // \___ \| |_) / _ \ \ /\ / /|  \| |
        //  ___) |  __/ ___ \ V  V / | |\  |
        // |____/|_| /_/   \_\_/\_/  |_| \_|

    
        children_stuff();
        if (mcmpi_hostfile_spawned == 1){
            sem_wait(&sem_wait_exit_prologue);
        }
    } // else parent == NULL

    //  ____    _    ____  ____  ___ _____ ____
    // | __ )  / \  |  _ \|  _ \|_ _| ____|  _ \
    // |  _ \ / _ \ | |_) | |_) || ||  _| | |_) |
    // | |_) / ___ \|  _ <|  _ < | || |___|  _ <
    // |____/_/   \_\_| \_\_| \_\___|_____|_| \_\
    
    // Meaning controller has finished spawning mcmpi_hostfile
    init = 0;
    skip_route = 0;
    if (mcmpi_hostfile_spawned)
        MPI_XBarrier(mcmpi_comm);
    return rs;
}


#include "aux_mcmpi_funcs.c"
#include "aux_mpi_funcs.c"
#include "aux_mpi_xfuncs.c"


void stripnewline(char * line){
    int i = 0;
    while(line[i] != '\0'){
        if (line[i] == '\n'){
            line[i] = '\0';
            break;
        }
        i++;
    }
}

int is_cluster_gateway(char * hostname){
    int i;
    int index = -1;
    for(i = 0; i < cmds.size; i++){
        if (strcmp(cmds.key[i], hostname) == 0){
            index = i;
            break;
        }
    }
    return index;
}

int is_cluster_group(char * line){
    char clustergroup[] = "[cluster|";
    int i;
    for (i = 0; i < strlen(clustergroup); i++){
        if (clustergroup[i] != line[i])
            return 0;
    }
    return 1;
}

char * getparent(char * line){
    char * rs = (char *) malloc(sizeof(char) * 500);
    int i = 9;
    int rsindex = 0;
    while (line[i] != '\0'){
        if (line[i] == ']') break;
        rs[rsindex] = line[i];
        rsindex++;
        i++;
    }
    rs[rsindex] = '\0';
    return rs;
}

void gethostproc(char * line, char * hostname, int * proc){
    char * procstring = (char *) malloc(sizeof(char)*10);
    int i = 0;
    int index = 0;
    char * tostring = hostname;
    while (line[i] != '\0'){
        if (line[i] == ' '){
            tostring[index] = '\0';
            tostring = procstring;
            index = 0;
            i++;
            continue;
        }
        tostring[index] = line[i];
        index++;
        i++;
    }
    tostring[index] = '\0';

    *proc = atoi(procstring);
    return;
}

void dump_nodes(struct Nodes *nodes){
    int i;
    VB(("Total of %d nodes\n", nodes->size));
    for (i = 0; i < nodes->size; i++){
        VB(("| hostname = %-15s type = %d local_rank = %2d global_rank = %2d comm = %-10s\n", nodes->hostname[i], nodes->type[i], nodes->local_rank[i], nodes->global_rank[i], nodes->comm[i]));
    }
}

void dump_nodes2(struct Nodes *nodes){
    int i;
    printf("Total of %d nodes\n", nodes->size);
    for (i = 0; i < nodes->size; i++){
        printf("| hostname = %-15s type = %d local_rank = %2d global_rank = %2d comm = %-10s\n", 
                nodes->hostname[i], 
                nodes->type[i], 
                nodes->local_rank[i], 
                nodes->global_rank[i], 
                nodes->comm[i]);
    }
}


void append_command(struct Commands * cmds, char * hostname, int proc, char * comm){
    int i;
    int found_index = -1;
    for (i = 0; i < cmds->size; i++){
        if (strcmp(cmds->key[i], comm) == 0){
            found_index = i;
            break;
        }
    }
    // Global
    //   cmds.key[0] = "global"
    //   cmds.list[0] = "server1:2,server2:2"
    //   cmds.total_proc[0] = 4 (2+2)
    // ClusterUSP
    //   cmds.key[1] = "clusterusp"
    //   cmds.list[1] = "node01:2,node02:4"
    //   cmds.total_proc[1] = 6 (2+4)
    if (found_index == -1){
        strcpy(cmds->key[cmds->size], comm);
        sprintf(cmds->list[cmds->size], "%s:%d", hostname, proc);
        cmds->total_proc[cmds->size] += proc;
        cmds->size++;
    }else{
        int tempsize = strlen(cmds->list[found_index]);
        sprintf(&cmds->list[found_index][tempsize], ",%s:%d", hostname, proc);
        cmds->total_proc[found_index] += proc;
    }
}

void init_commands(struct Commands *cmds){
    int i, j;
    cmds->size = 0;
    for (i = 0; i < 500; i++){
        cmds->total_proc[i] = 0;
        for (j = 0; j < 500; j++){
            cmds->key[i][j] = '\0';
        }
    }
    for (i = 0; i < 500; i++){
        for (j = 0; j < 5000; j++){
            cmds->list[i][j] = '\0';
        }
    }
}

void dump_commands(struct Commands *cmds){
    int i;
    for (i = 0; i < cmds->size; i++){
        if (DEBUG || 1) printf("[%d] %s => %s\n", cmds->total_proc[i], cmds->key[i], cmds->list[i]);
    }
}

void read_mcmpi_file(char * mcmpi_hostfile){
    FILE * fp;
    char * line = NULL;
    size_t  len = 0;
    ssize_t read;


    fp = fopen(mcmpi_hostfile, "r"); if (fp == NULL)
        exit(1);

    char * tempparent = "global";
    char globalstring[] = "[global]";
    char clusterstring[] = "[cluster|";
    int temptype = 0;
    char * hostname = (char *) malloc(sizeof(char)*500);
    int * proc = (int *) malloc(sizeof(int));
    init_commands(&cmds);
    while ((read = getline(&line, &len, fp)) != -1){
        stripnewline(line);
        if (line[0] == '#') continue;

        if (strlen(line) == 0)
            continue;
        if (strcmp(line, globalstring) == 0){
            temptype = 0;
            continue;
        }
        if (is_cluster_group(line) == 1){
            tempparent = getparent(line);
            if (0){
                strcpy(nodes.hostname[nodes.size], tempparent);
                nodes.type[nodes.size] = 1;
                nodes.local_rank[nodes.size] = -1;
                nodes.global_rank[nodes.size] = -1;
                strcpy(nodes.comm[nodes.size], tempparent);
                nodes.size++;
            }
            continue;
        }
        gethostproc(line, hostname, proc);
        append_command(&cmds, hostname, *proc, tempparent);
        int i;
        for (i = 0; i < *proc; i++){
            if (0){
                strcpy(nodes.hostname[nodes.size], hostname);
                nodes.type[nodes.size] = 0;
                nodes.local_rank[nodes.size] = -1;
                nodes.global_rank[nodes.size] = -1;
                strcpy(nodes.comm[nodes.size], tempparent);
                nodes.size++;
            }
        }
    }
    fclose(fp);
    if (line)
        free(line);
}

void init_gateway(){
    send_command_arg_lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(send_command_arg_lock, NULL);
}

void spawn_remote_mpi2(char * hostname, int proc, char * hosts){
    //  ____    _  _____ _______        ___ __   __  _
    // / ___|  / \|_   _| ____\ \      / / \\ \ / / / |
    //| |  _  / _ \ | | |  _|  \ \ /\ / / _ \\ V /  | |
    //| |_| |/ ___ \| | | |___  \ V  V / ___ \| |   | |
    // \____/_/   \_\_| |_____|  \_/\_/_/   \_\_|___|_|
    //                                         |_____|

    init_gateway();
    char mpi_command[5000];
    char outputfile[200];
    sprintf(outputfile, "%s-remotempi.log", hostname);

    VB(("create comm buffers\n"));
    char * comm_buffers = create_comm_buffers2();
    VB(("init comm buffers\n"));
    init_comm_buffers();
    int i = 0;
    int tid[2] = {0,1};
    pthread_create(&tid_task_loop_worker, 
                   NULL, 
                   task_loop_worker_thread, 
                   NULL);
    for (i = 0; i < 2; i++)
        if (pthread_create(&pid_inout[i], NULL, inout_thread, &tid[i])){
            if (DEBUG) printf("error creating thread\n");
        }
    sleep(5);
    char * preload = getenv("LD_PRELOAD");
    char cwd[1000];
    getcwd(cwd, sizeof(cwd));
    popen("systemctl kill -s SIGKILL mcmpi; systemctl reset-failed;  sleep 2", "r");
    VB(("to spawn\n"));
    sleep(3);

    sprintf(mpi_command, "systemd-run --unit='mcmpi' \
            -p LimitSTACK=infinity \
            bash -c \"\
            source /etc/profile.d/mcmpi.sh && \
            cd %s && \
            mpirun --allow-run-as-root \
                -n %d \
                -host %s:1,%s \
                -x LD_PRELOAD='%s' \
                -x MCMPI_CLUSTER=true \
                -x MCMPI_APP=true \
                -x MCMPI_BUFFERS='%s' \
                ./hello 2>&1 | tee %s\"", 
                    cwd,
                    proc+1, 
                    hostname, 
                    hosts, 
                    preload, 
                    comm_buffers, 
                    outputfile);
    popen(mpi_command, "r");

    if (pthread_create(&tid_remote_mpi_output, NULL, listen_svc, NULL)){
        if (DEBUG) printf("Cannot create thread remote mpi\n");
        if (DEBUG) fflush(stdout);
        exit(1);
    }
    sleep(3);
    VB(("to receive grank_start\n"));
    MPI_Recv(&grank_start, 1, MPI_INT, 0, 0,  mcmpi_comm, &status);
    VB(("received grank_start from controller %d\n", grank_start));
    VB(("send grank_command\n"));
    send_command_arg(2, grank_start, 1, 2, 3, 4, 5);
    for (i = 0; i < 2; i++)
        if (pthread_join(pid_inout[i], NULL)){
            if (DEBUG) printf("error joining thread\n");
        }
    if (pthread_join(tid_remote_mpi_output, NULL)){
        if (DEBUG) printf("error joining thread\n");
    }
    MPI_Finalize();
    exit(0);
}

void listen_svc(){
    FILE * fp = popen("journalctl -u mcmpi -f", "r");
    char line[1000];
    while (fgets(line, sizeof(line), fp) != NULL){
        printf("[sys] %s", line);
    }
}

void spawn_remote_mpi(char * hostname, int proc, char * hosts, int grank){

    //  ____    _  _____ _______        ___ __   __  _
    // / ___|  / \|_   _| ____\ \      / / \\ \ / / / |
    //| |  _  / _ \ | | |  _|  \ \ /\ / / _ \\ V /  | |
    //| |_| |/ ___ \| | | |___  \ V  V / ___ \| |   | |
    // \____/_/   \_\_| |_____|  \_/\_/_/   \_\_|___|_|
    //                                         |_____|

    init_gateway();
    char mpi_command[5000];
    char outputfile[200];
    sprintf(outputfile, "%s-remotempi.log", hostname);

    create_comm_buffers(hostname);
    init_comm_buffers();
    int i = 0;
    int tid[2] = {0,1};
    for (i = 0; i < 2; i++)
        if (pthread_create(&pid_inout[i], NULL, inout_thread, &tid[i])){
            if (DEBUG) printf("error creating thread\n");
        }
    sleep(10);
    sprintf(mpi_command, "systemd-run bash -c \"source /etc/profile.d/lmod.sh && source /etc/profile.d/omp.sh && source /etc/profile.d/mcmpi.sh && cd /code/step1 && LD_PRELOAD=/code/step1/preload.so && MCMPI_CLUSTER=true MCMPI_APP=true mpirun --allow-run-as-root -n %d -host %s:1,%s -x MCMPI_CLUSTER -x MCMPI_APP ./hello 2>&1 | tee %s\"", proc+1, hostname, hosts, outputfile);
    FILE *fp;
    fp = popen(mpi_command, "r");
    if (fp == NULL){
        if (DEBUG) printf("Failed to run command\n");
        if (DEBUG) fflush(stdout);
    }

    send_command_arg(6, global_rank, 0, 0, 0, 0, 0);
    send_command_arg(2, grank, 0, 0, 0, 0, 0);
    init = 1;
    for (i = 0; i < 2; i++)
        if (pthread_join(pid_inout[i], NULL)){
            if (DEBUG) printf("error joining thread\n");
        }
    if (pthread_join(tid_remote_mpi_output, NULL)){
        if (DEBUG) printf("error joining thread\n");
    }
    MPI_Finalize();
    exit(0);
}
void call_listen_file(void *vargp){
    char * filename = (char*) vargp;
    listen_file(filename);
}
void listen_file(char * filename){
    FILE * fp;
    char mystring [1000];
    fp = fopen (filename , "r");
    char ch;
    for (;;) {
        while ((ch = getc(fp)) != EOF)  {
            if (putchar(ch) == EOF)
                perror("Output error");
        }
        if (ferror(fp)) {
            if (DEBUG) printf("Input error: %s", errno);
            return;
        }
        (void)fflush(stdout);
        sleep(1); // Or use select
    }
    fclose (fp);
    return;
}

void dumpnodes2file(char * filename){
    FILE * fp;
    int i;
    fp = fopen(filename, "w");
    for(i = 0; i < nodes.size; i++){
        fprintf(fp, "%s,%d,%d,%d,%s\n", nodes.hostname[i], nodes.type[i], nodes.local_rank[i], nodes.global_rank[i], nodes.comm[i]);
    }
    fclose(fp);
}
void add_node(char * hostname, int type, int local_rank, int global_rank, char * comm){
    strcpy(nodes.hostname[nodes.size], hostname);
    nodes.type[nodes.size] = type;
    nodes.local_rank[nodes.size] = local_rank;
    nodes.global_rank[nodes.size] = global_rank;
    strcpy(nodes.comm[nodes.size], comm);
    nodes.size++;
}
void load_cluster_nodes_from_file_fly(char * filename, char * hostname){
    FILE * fp;
    char * line = NULL;
    size_t  len = 0;
    ssize_t read;

    fp = fopen(filename, "r");
    if (fp == NULL)
        exit(1);


    int i,j;

    while ((read = getline(&line, &len, fp)) != -1){
        stripnewline(line);
        if (strlen(line) == 0)
            continue;

        char messages[5][1000];
        for(i = 0; i < 5; i++)
            for(j = 0; j < 1000; j++)
                messages[i][j] = '\0';

        int iter = 0;
        int line_index = 0;
        char tempmessage[1000];
        int temp_index = 0;
        for (i = 0; i < 1000; i++) tempmessage[i] = '\0';
        while(line[line_index] != '\0'){
            if (line[line_index] == ','){
                strcpy(messages[iter], tempmessage);
                temp_index = 0;
                iter++;
                line_index++;
                for (i = 0; i < 1000; i++) tempmessage[i] = '\0';
            }
            tempmessage[temp_index] = line[line_index];
            temp_index++;
            line_index++;
        }

        strcpy(messages[iter], tempmessage);
        if (strcmp(messages[0], hostname) == 0 || strcmp(messages[4],hostname) == 0)
            edit_node(messages[0], atoi(messages[2]), atoi(messages[3]));
    }

}
void load_cluster_nodes_from_file(char * filename, char * hostname){
    FILE * fp;
    char * line = NULL;
    size_t  len = 0;
    ssize_t read;

    fp = fopen(filename, "r");
    if (fp == NULL)
        exit(1);


    int i,j;

    while ((read = getline(&line, &len, fp)) != -1){
        stripnewline(line);
        if (strlen(line) == 0)
            continue;

        char messages[5][1000];
        for(i = 0; i < 5; i++)
            for(j = 0; j < 1000; j++)
                messages[i][j] = '\0';

        int iter = 0;
        int line_index = 0;
        char tempmessage[1000];
        int temp_index = 0;
        for (i = 0; i < 1000; i++) tempmessage[i] = '\0';
        while(line[line_index] != '\0'){
            if (line[line_index] == ','){
                strcpy(messages[iter], tempmessage);
                temp_index = 0;
                iter++;
                line_index++;
                for (i = 0; i < 1000; i++) tempmessage[i] = '\0';
            }
            tempmessage[temp_index] = line[line_index];
            temp_index++;
            line_index++;
        }

        strcpy(messages[iter], tempmessage);
        if (strcmp(messages[0], hostname) == 0 || strcmp(messages[4],hostname) == 0)
            add_node(messages[0], atoi(messages[1]), atoi(messages[2]), atoi(messages[3]), messages[4]);
    }

}
void load_nodes_from_file(char * filename){
    FILE * fp;
    char * line = NULL;
    size_t  len = 0;
    ssize_t read;

    nodes.size = 0;


    fp = fopen(filename, "r");
    if (fp == NULL)
        exit(1);


    int i,j;

    while ((read = getline(&line, &len, fp)) != -1){
        stripnewline(line);
        if (strlen(line) == 0)
            continue;

        char messages[5][1000];
        for(i = 0; i < 5; i++)
            for(j = 0; j < 1000; j++)
                messages[i][j] = '\0';

        int iter = 0;
        int line_index = 0;
        char tempmessage[1000];
        int temp_index = 0;
        for (i = 0; i < 1000; i++) tempmessage[i] = '\0';
        while(line[line_index] != '\0'){
            if (line[line_index] == ','){
                strcpy(messages[iter], tempmessage);
                //printf("%s\n", messages[iter]);
                temp_index = 0;
                iter++;
                line_index++;
                for (i = 0; i < 1000; i++) tempmessage[i] = '\0';
            }
            tempmessage[temp_index] = line[line_index];
            temp_index++;
            line_index++;
        }
        strcpy(messages[iter], tempmessage);

        add_node(messages[0], atoi(messages[1]), atoi(messages[2]), atoi(messages[3]), messages[4]);

    }

}

void edit_node(char * hostname, int local_rank, int global_rank){
    int i;
    for (i = 0; i < nodes.size; i++){
        if (strcmp(nodes.hostname[i], hostname) == 0){
            if(nodes.local_rank[i] == -1 || nodes.local_rank[i] == local_rank){
                nodes.local_rank[i] = local_rank;
                nodes.global_rank[i] = global_rank;
                return;
            }
        }
    }
}

void load_nodes_from_file_fly(char * filename){
    FILE * fp;
    char * line = NULL;
    size_t  len = 0;
    ssize_t read;


    fp = fopen(filename, "r");
    if (fp == NULL)
        exit(1);

    int i,j;

    while ((read = getline(&line, &len, fp)) != -1){
        stripnewline(line);
        if (strlen(line) == 0)
            continue;

        char messages[5][1000];
        for(i = 0; i < 5; i++)
            for(j = 0; j < 1000; j++)
                messages[i][j] = '\0';

        int iter = 0;
        int line_index = 0;
        char tempmessage[1000];
        int temp_index = 0;
        for (i = 0; i < 1000; i++) tempmessage[i] = '\0';
        while(line[line_index] != '\0'){
            if (line[line_index] == ','){
                strcpy(messages[iter], tempmessage);
                temp_index = 0;
                iter++;
                line_index++;
                for (i = 0; i < 1000; i++) tempmessage[i] = '\0';
            }
            tempmessage[temp_index] = line[line_index];
            temp_index++;
            line_index++;
        }
        strcpy(messages[iter], tempmessage);

        edit_node(messages[0], atoi(messages[2]), atoi(messages[3]));
    }

}

#include "aux_buffer.c"

void save_2file(char * filename, char * str){
    FILE * fp;
    if (DEBUG) fflush(stdout);
    fp = fopen(filename, "w");
    fprintf(fp, "%s", str);
    fclose(fp);
}

char * load_file(char * filename){
    char * line = (char*) malloc(sizeof(char)*1000);
    FILE * fp;
    fp = fopen(filename, "r");
    fgets(line, 1000, fp);
    fclose(fp);
    return line;
}
void update_from_node_table(){
    int i;
    global_size = 0;
    for (i = 0; i < nodes.size; i++){
        if (nodes.type[i] == 0)
            global_size++;
    }
}
void warn(char * msg){
    if (DEBUG) printf("===========================================================================================================================\n");
    if (DEBUG) fflush(stdout);
    if (DEBUG) printf("%*s%s\n", 18, " ", msg);
    if (DEBUG) fflush(stdout);
    if (DEBUG) printf("===========================================================================================================================\n");
    if (DEBUG) fflush(stdout);
}
void rebuild_node_table(){
    nodes.size = 0;
    if (DEBUG) printf("[%s] Loading final node table\n", processor_name);
    if (DEBUG) fflush(stdout);
    load_nodes_from_file("mcmpi-nodetable");
    dump_nodes(&nodes);
}

struct Node * fetch_node_from_gr(int global_rank){
    int i;
    struct Node * node = (struct Node*) malloc(sizeof(struct Node));
    for(i = 0; i < nodes.size; i++){
        if (nodes.global_rank[i] == global_rank){
            strcpy(node->hostname, nodes.hostname[i]);
            node->type = nodes.type[i];
            node->local_rank = nodes.local_rank[i];
            node->global_rank = nodes.global_rank[i];
            strcpy(node->comm, nodes.comm[i]);
            break;
        }
    }
    return node;
}
struct Node * fetch_node_from_hostname(char * hostname){
    //warn("fetch node from gr");
    int i;
    struct Node * node = (struct Node*) malloc(sizeof(struct Node));
    for(i = 0; i < nodes.size; i++){
        if (strcmp(hostname, nodes.hostname[i]) == 0){
            strcpy(node->hostname, nodes.hostname[i]);
            node->type = nodes.type[i];
            node->local_rank = nodes.local_rank[i];
            node->global_rank = nodes.global_rank[i];
            strcpy(node->comm, nodes.comm[i]);
            break;
        }
    }
    return node;
}
struct Node * fetch_gateway_comm_node(char * comm){
    if (DEBUG) warn("fetch gateway node from global rank");
    int i;
    struct Node * node = (struct Node*) malloc(sizeof(struct Node));
    for(i = 0; i < nodes.size; i++){
        if (strcmp(nodes.hostname[i], comm) == 0 && nodes.type[i] == 1){
            strcpy(node->hostname, nodes.hostname[i]);
            node->type = nodes.type[i];
            node->local_rank = nodes.local_rank[i];
            node->global_rank = nodes.global_rank[i];
            strcpy(node->comm, nodes.comm[i]);
            break;
        }
    }
    return node;
}
void show_node_info(struct Node * node){
       if (DEBUG) printf("Node info: %s %d %d %d %s\n", node->hostname, node->type, node->local_rank, node->global_rank, node->comm); fflush(stdout);
}

void show_node_info2(char * prefix, struct Node * node){
    VB(("%s %s, t %d, lr %d, gr %d, %s\n",
                prefix,
                node->hostname,
                node->type,
                node->local_rank,
                node->global_rank,
                node->comm));
}


#include "aux_nodetask.c"


char ** split_servers(char *line, int * count){

    char **ss = malloc(sizeof(char*) * 100);
    int i,j,x,y,a,b;
    for (i = 0; i < 10; i++){
        ss[i] = malloc(sizeof(char) * 20);
        for (y = 0; y < 20; y++)
            ss[i][y] = '\0';

    }


    *count = 0;

    int start = 0;
    int start_colon = 0;
    int slen = strlen(line);
    char temp_server[20];
    char temp_proc[20];

    for (i = 0; i < slen; i++){
        if (line[i] == ',' || i == slen-1){
            for (y = 0; y < 20; y++) temp_proc[y] = 0;
            if (i == start_colon) i++;
            strncpy(temp_proc, line+start_colon, i-start_colon);
            int nproc = atoi(temp_proc);
            for (y = 0; y < nproc; y++){
                strcpy(ss[*count], temp_server);
                (*count)++;
            }
            start = i+1;
        }
        if (line[i] == ':'){
            for (y = 0; y < 20; y++) temp_server[y] = 0;
            strncpy(temp_server, line+start, i-start);
            start_colon = i+1;
        }
    }
    return ss;
}


int gr2lr(int arg_grank){
    int i;
    for (i = 0; i < nodes.size; i++){
        if (nodes.global_rank[i] == arg_grank)
            return i;
    }
    return -1;
}

int get_grank_index(int arg_grank){
    int i;
    for (i = 0; i < nodes.size; i++){
        if (nodes.global_rank[i] == arg_grank)
            return i;
    }
    return -1;
}

int count_gateway_nodes(){
    int res, i;
    res = 0;
    for (i = 0; i < nodes.size; i++){
        if (nodes.type[i] == 1) res++;
    }
    return res;
}

void show_action_info(int * action){
    //int action[6] = {0,msg_type_size,msg_size,msg_tag,msg_src,msg_dest};
    VB(("msg_type_size(%d) msg_size(%d) msg_tag(%d) msg_src(%d) msg_dest(%d)\n",
                action[1],
                action[2],
                action[3],
                action[4],
                action[5]));
}

#include "add_node.c"
