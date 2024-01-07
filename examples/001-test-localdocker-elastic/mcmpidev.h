// App
void parse_cluster_resp(char * resp, int * global_rank, int * total_proc, int * tgrank, char * command);
// spawn_remote_mpi mcmpi old
void spawn_remote_mpi(char * hostname, int proc, char * hosts, int grank);
// spawn_remote_mpi mcmpi wscad21
void spawn_remote_mpi2(char * hostname, int proc, char * hosts);
pthread_t tid_remote_mpi_output;
pthread_t tid_task_loop_worker;
pthread_t tid_thread_work;
void remove_rank(int lower, int upper);
int count_gateway_nodes();
void task_loop_worker_thread();
void call_listen_file(void *vargp);
void listen_file(char * filename);
void update_from_node_table();
void warn(char * msg);
void init_gateway(); // init gateway semaphores. 

int isremoving = 0;

// MCMPI in aux_mcmpi_funcs.c
void mcmpi_scale_on();
void mcmpi_scale_off();
void mcmpi_remove_rank(int whichrank);
void mcmpi_remove_nranks(int size);
void mcmpi_remove_cluster(char * clustername);
void _mcmpi_remove_rank(int lower, int upper);
int mcmpi_scale_on_val = 1;

// Readfile
struct Nodes {
    char hostname[500][500];
    int type[500]; // 0 node; 1 gateway
    int local_rank[500]; 
    int global_rank[500];
    char comm[500][500]; // global or cluster name
    int size; // all size
    int grank_size; // only granks. USER COMM
};

struct Commands{
    char key[500][500]; // global or cluster name
    char list[500][5000]; // server1:2,server2:3
    int total_proc[500]; // 5
    int size; // SoA idx
};


void stripnewline(char * line);
int is_cluster_group(char * line);
char * getparent(char * line);
void gethostproc(char * line, char * hostname, int * proc);
void dump_nodes(struct Nodes *nodes);
void dump_nodes2(struct Nodes *nodes); // force bypass VB printf
void dump_commands(struct Commands *cmds);
void init_commands(struct Commands *cmds);
int is_cluster_gateway(char * hostname);
void read_mcmpi_file();

// Load file
void dumpnodes2file(char * filename);
void add_node(char * hostname, int type, int local_rank, int global_rank, char * comm);
void load_cluster_nodes_from_file(char * filename, char * hostname);
void load_nodes_from_file(char * filename);
void edit_node(char * hostname, int local_rank, int global_rank);
void load_nodes_from_file_fly(char * filename);
void load_cluster_nodes_from_file_fly(char * filename, char * hostname);
void rebuild_node_table(); // clear nodes struct and rebuild from mcmpi-nodetable file

// Buffer
void escreve(int carac);
int leia();
void produz();
void produz2();
void consume();
int * parse_buffer_command(char * str); // explode char '-'
void * inout_thread(void *v);
void * inout_thread2(void *v);
void save_2file(char * filename, char * str);
char * load_file(char * filename);
// comm_buffers
    // new
    char * create_comm_buffers2();
    void create_comm_buffers_from_env();
    // old
    void create_comm_buffers(char * hostname);
    void create_comm_buffers_from_file(char * hostname);
//
void init_comm_buffers();
void listen_svc(); // journalctl -u mcmcpi-run -f
void exec_command(); // exec command received from consome();
char * genmessage(int action, int type, int size, int tag, int dest, int buffer_id);
void send_command_arg(int action,  // 7 args
                      int type_size, 
                      int size, 
                      int tag, 
                      int src,
                      int dest, 
                      int buffer_id);
// Router
struct Node {
    char hostname[500];
    int type;
    int local_rank;
    int global_rank;
    char comm[500];
};
struct Node * fetch_node_from_gr(int global_rank); //gr = global rank
struct Node * fetch_node_from_hostname(char * hostname);
struct Node * fetch_gateway_comm_node(char * comm);
void show_node_info(struct Node * node);
void show_action_info(int * action);

// Linked List for MPI_Isend on Gateways
struct NodeTask {
    int id;
    int id_buffer; 
    MPI_Request * req;
    int src;
    int dest;
    int tag;
    struct NodeTask * next;
};
int nodetasksize = 0;
void task_push(int id_buffer, MPI_Request * req, int src, int dest, int tag);
void task_print_list();
void task_loop_clean();

int MPI_NRecv(void *buf, int count, MPI_Datatype type, int source, int tag, 
        MPI_Comm comm, MPI_Status *status);
int MPI_NSend(const void *buf, int count, MPI_Datatype type, int dest,
        int tag, MPI_Comm comm);
int MPI_XRecv(void *buf, int count, MPI_Datatype type, int source, int tag, 
        MPI_Comm comm, MPI_Status *status);
int MPI_XSend(const void *buf, int count, MPI_Datatype type, int dest,
        int tag, MPI_Comm comm);
int MPI_XBarrier(MPI_Comm comm);
int MPI_NComm_rank(MPI_Comm comm, int *rank);
int MPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status);

