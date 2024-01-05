


MPI_Comm client_comm;

int todelete = -1;
int removebarrier = 0;

char *rtrim(char *s);
void client_work();
void controller_work();
void service_work();
char ** spawn_self(int n);
char ** spawn_ec2(int n);
char ** spawn_container(int n);

void add_consumer(int n);
void del_consumer(int n);
void print_consumers();
int count_consumers();

// client
pthread_t tid_client_recv;
void *client_recv(void *arg);
#define WKSIZE 2000
int timestart[WKSIZE];


// gateway
pthread_t tid_gateway;
pthread_t tid_response;
pthread_t tid_deliver;
pthread_t tid_monitor;
pthread_t tid_monitor_dbg;
void *controller_gateway(void *arg);
void *controller_response(void *arg);
void *controller_deliver(void *arg);
void *controller_monitor(void *arg);
void *controller_monitor_dbg(void *arg);

// Linked List for MPI_Isend on Gateways
struct NodeReq {
    int id;
    int start_ts; 
    struct NodeReq * next;
};
int reqsize = 0;
sem_t req_lock;
struct NodeReq * head_request;
struct NodeReq * tail_request;
void req_del(int id);
void req_push(int id, int start_ts);

struct NodeConsumer {
    int id;
    struct NodeConsumer * next;
};
sem_t consumer_lock;
sem_t consumer_avail;
int consumersize = 0;
struct NodeConsumer * head_consumer;
struct NodeConsumer * tail_consumer;
void consumer_del(int id);
void consumer_push(int id);

// remove rank semaphores
sem_t sem_remove;
sem_t sem_communication;
sem_t excl_count_communication;
int count_communication = 0;
void thread_work_remove_rank_sem_prologue();
void thread_work_remove_rank_sem_epilogue();
void mpi_remove_rank_sem_prologue();
void mpi_remove_rank_sem_epilogue();
void remove_rank_sem_init();
void remove_rank_sem_finalize();



