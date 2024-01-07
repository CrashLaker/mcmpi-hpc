#include <mpi.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/times.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h> // for sleep
#include <math.h>
#include <stdlib.h>
#include "mcmpi.h"
#include "aux_myapp.h"
#define VERBOSE 1
#define VB3(a) if (VERBOSE) { printf("[%lu][%s][%d] ", (unsigned long)time(NULL), hostname, myrank); printf a ; fflush(stdout); }
//#define VB3(a) if (VERBOSE) { printf("[%lu][%d] ", (unsigned long)time(NULL), myrank); printf a ; fflush(stdout); }
int myrank,numprocs;  //YYY incluido
int removebarrierdeliver;
int removebarrierresponse;
char hostname[100]; //YYY incluido
void del_many_consumers(int n);
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
    int sleepm = 800;
    for (i = 0; i < WKSIZE; i++){
        //VB3(("To send reqid %d\n", i));
        timestart[i] = (int)time(NULL);
        MPI_Isend(&i,
                  1,
                  MPI_INT,
                  0,
                  50,
                  //XXX mcmpi_comm,
                  MPI_COMM_WORLD,
                  &treq); 
        usleep(sleepm*1000);
        if (i == 60) sleep(100000);
        if (i == 300) sleepm = 500;
        if (i == 800) sleepm = 200;
    }
    if (pthread_join(tid_client_recv,NULL)){
        printf("tid_client_recv join error\n");
    }
}

void *client_recv(void *arg){
//    VB3(("THREAD Service Recv UP\n"));

    int flag, tag, src, idx;
    MPI_Status probestatus;
    while (1){
        flag = 0;
        while (!flag){
            MPI_Iprobe(MPI_ANY_SOURCE,
                       50,
                       //XXX mcmpi_comm,
                       MPI_COMM_WORLD,
                       &flag,
                       &probestatus);
            if (flag) break;
        }
        tag = probestatus.MPI_TAG;
        src = probestatus.MPI_SOURCE;
        int reqid;
        MPI_NRecv(&reqid,
                 1,
                 MPI_INT,
                 src,
                 tag,
                 //XXX mcmpi_comm,
                MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE
                );
        //VB3(("service recv received reqid %d\n", reqid));
        int end_ts = (int)time(NULL);
        VB3(("request %d finished in %d seconds\n", reqid, 
                    end_ts - timestart[reqid]));
    }
}

void controller_work(){
    //VB3(("TODO controller work\n"));

    int i;
    //XXX for (i = 0; i < 200; i++)
    //XXX    monstate[i] = '\0';

    int spawn_error;

    MPI_Info client_info;
    MPI_Info_create(&client_info);
    //MPI_Info_set(client_info, "add-host", "client");
    MPI_Info_set(client_info, "host", "client");
    MPI_Comm_spawn("./hello-client",
                   MPI_ARGV_NULL,
                   1,
                   //MPI_INFO_NULL,
                   client_info,
                   0,
                   MPI_COMM_WORLD,
                   &client_comm,
                   &spawn_error);

    int cinit;
    MPI_Status cinit_status;
    MPI_NRecv(&cinit, 1, MPI_INT, 0, 0, client_comm, &cinit_status);
  //  VB3(("client is up! %d\n", cinit));
    
    VB3((">> START controller_gateway\n"));
    if (pthread_create(&tid_gateway, 
                       NULL, 
                       controller_gateway, 
                       NULL)){
        printf("tid_gateway error\n");
    } 

    VB3((">> START controller_response\n"));
    if (pthread_create(&tid_response, 
                       NULL, 
                       controller_response, 
                       NULL)){
        printf("tid_response error\n");
    } 

    VB3((">> START controller_deliver\n"));
    if (pthread_create(&tid_deliver, 
                       NULL, 
                       controller_deliver, 
                       NULL)){
        printf("tid_deliver error\n");
    } 

    VB3((">> START controller_monitor\n"));
    if (pthread_create(&tid_monitor, 
                       NULL, 
                       controller_monitor, 
                       NULL)){
        printf("tid_monitor error\n");
    } 

    VB3((">> START controller_monitor_dbg\n"));
    if (pthread_create(&tid_monitor_dbg, 
                       NULL, 
                       controller_monitor_dbg, 
                       NULL)){
        printf("tid_monitor error\n");
    } 


    if (pthread_join(tid_gateway,NULL)){
        printf("tid_gateway join error\n");
    }
    if (pthread_join(tid_response,NULL)){
        printf("tid_response join error\n");
    }
    if (pthread_join(tid_deliver,NULL)){
        printf("tid_deliver join error\n");
    }
    if (pthread_join(tid_monitor,NULL)){
        printf("tid_monitor join error\n");
    }
    if (pthread_join(tid_monitor_dbg,NULL)){
        printf("tid_monitor join error\n");
    }

}

void *controller_monitor_dbg(void *arg){
    
    int workersize;
    while (1){
        //workersize = nodes.grank_size - 2;// rm controller client
        //XXX workersize = nodes.grank_size - 1;// rm controller
        workersize = numprocs - 1;// rm controller
      //  VB3(("[mondbg] -----------MON DBG START-----------\n"));
      //  VB3(("[mondbg] >>> Current queue size: %d\n", reqsize)); 
      //  VB3(("[mondbg] >>> Current consumer/workersize count: %d/%d\n", count_consumers(), workersize)); 
      //  VB3(("[mondbg] >>> removebarrierdeliver: %d\n", removebarrierdeliver)); 
     //   VB3(("[mondbg] >>> removebarrierresponse: %d\n", removebarrierresponse)); 
    //    VB3(("[mondbg] >>> mon state: %s\n", monstate)); 
        //WWW  excluido dump_nodes2(&nodes);
        if (head_consumer == NULL){
      //      VB3(("[mondbg] consumer queue is empty\n")); 
        }else{
            struct NodeConsumer * current;
            current = head_consumer;
            while (current){
        //        VB3(("[mondbg] >>> --> req %d\n", current->id));
                current = current->next;
            }
        }
        sleep(5);
    }

}

void *controller_monitor(void *arg){
    // consumersize // consumer size
    // reqsize // queue length
    //sleep(120);
    sleep(10);
    float ratioUsage, perConsumer;
    float perConsumerTarget = 10; // target is max 10 requests per consumer
                                  // in queue
    float consumerTarget;
    int i, tospawn, toremove;
    struct NodeConsumer * current;
    int workersize;
    int csize;
/* ZZZ a ser excluido
    if (0){
        strcpy(monstate, "to spawn 5 consumers ==========================================");
        add_consumer(5);
        //WWW excluido dump_nodes2(&nodes);
        sleep(4);
        workersize = nodes.grank_size - 1;// rm controller


        strcpy(monstate, "to remove 4 ranks ==========================================");
       // VB3(("now to remove\n"));
       // VB3(("[mon] --- To remove global_rank %d\n", nodes.grank_size-1));
        removebarrierdeliver = 1;
        csize = count_consumers();
        while(workersize != csize){
         //   VB3(("[mon] waiting for workersize %d == consumer queue size %d\n", workersize, csize));
            sleep(1); 
            csize = count_consumers();
        }
        removebarrierresponse = 1;
        sleep(3);
        //VB3(("[mon] finished waiting for workersize %d == consumer queue size %d\n", workersize, csize));
        //del_consumer(nodes.grank_size-1);
        del_many_consumers(4);
        removebarrierresponse = 0;
        removebarrierdeliver = 0;
        sleep(3);
        dump_nodes2(&nodes);
    }
    if (0){ 
        strcpy(monstate, "to spawn 3 consumers ==========================================");
        add_consumer(3);
        dump_nodes(&nodes);
        sleep(4);
        workersize = nodes.grank_size - 1;// rm controller

        strcpy(monstate, "to remove 1 rank ==========================================");
        //VB3(("now to remove\n"));
        //VB3(("[mon] --- To remove global_rank %d\n", nodes.grank_size-1));
        removebarrierdeliver = 1;
        csize = count_consumers();
        while(workersize != csize){
        //    VB3(("[mon] waiting for workersize %d == consumer queue size %d\n", workersize, csize));
            sleep(1); 
            csize = count_consumers();
        }
        removebarrierresponse = 1;
        sleep(3);
        VB3(("[mon] finished waiting for workersize %d == consumer queue size %d\n", workersize, csize));
        del_consumer(nodes.grank_size-1);
        removebarrierresponse = 0;
        removebarrierdeliver = 0;

        strcpy(monstate, "to add 1 consumer after remove\n");
        sleep(4);
      //  VB3(("now to add\n"));
        add_consumer(1);
        dump_nodes2(&nodes);
        sleep(4);

        strcpy(monstate, "to add 2 consumers last\n");
        sleep(4);
        //VB3(("now to add 2 more\n"));
        add_consumer(2);
        dump_nodes2(&nodes);
        sleep(4);

        strcpy(monstate, "finished tests\n");
    }
*/  //FIM de CODIGO a ser EXCLUIDO

    while (1){
        //workersize = nodes.grank_size - 2;// rm controller client
        //XXX workersize = nodes.grank_size - 1;// rm controller
        workersize = numprocs - 1;// rm controller
        //VB3(("[mon] -----------MON START-----------\n"));
        //VB3((">>> Current queue size: %d\n", reqsize)); 
        //VB3((">>> Current consumer count: %d\n", workersize)); 
        if (reqsize > 0 && workersize > 0){
            perConsumer = reqsize/workersize;
            VB3(("[mon] >>> perConsumer reqsize/workersize (%d/%d) = %f\n",
                            reqsize, workersize, perConsumer));
            ratioUsage = perConsumer/perConsumerTarget;
            consumerTarget = ceil(workersize*ratioUsage);
            //VB3(("[mon] ratioUsage: %f\n", ratioUsage));
            //VB3(("[mon] consumerTarget: %f\n", consumerTarget));
            current = head_consumer;
            while (current){
              //  VB3(("[mon] --- W%d\n", current->id));
                current = current->next;
            }
            if (consumerTarget > workersize){
                tospawn = consumerTarget - workersize;
                //VB3(("[mon] --- To spawn %d more nodes\n", tospawn));
                if (tospawn > 20) tospawn = 20;
                //VB3(("[mon] monstate:Spawning %d nodes\n", tospawn));
                add_consumer(tospawn);
                //WWW excluido dump_nodes(&nodes);
                //VB3(("[mon] monstate:Spawning %d nodes OK\n", tospawn));
                //for(i = 0; i < tospawn; i++){
                //    VB3(("--- Add consumer\n"));
                //}
            }else if (consumerTarget < workersize){ // to remove
                // workerSize = real. 12 
                // consumerTarget = 5
                // toremove 12 - 5 =7
                toremove =  workersize - consumerTarget;
                if (workersize < 6 && reqsize != 0){
                    sleep(5);
                    continue;
                }
                //if (toremove < 3){
                //    sleep(5);
                //    continue;
                //}
                if (workersize - toremove <= 1){
                    toremove = workersize - 1;
                }
                VB3(("[mon] --- To remove %d nodes\n", toremove));
                if (workersize > 1) {
                    //for(i = 0; i < toremove; i++){ 
                        if (reqsize == 0){
                            //VB3(("[mon] --- To remove global_rank %d\n", nodes.grank_size-3));
                            //del_consumer(nodes.grank_size-1);
                            //XXX toremove = nodes.grank_size-3;
                            toremove = numprocs-3;
                            //VB3(("[mon] monstate:Reqsize empty. Removing %d nodes\n", toremove));
                            if (toremove > 0)
                                del_many_consumers(toremove);
                            //VB3(("[mon] monstate:Reqsize empty. Removing %d nodes OK\n", toremove));
                        }else if (todelete == -1){
                            //VB3(("[mon] --- To remove global_rank %d\n", nodes.grank_size-1));
                            //VB3(("[mon] monstate:Removing %d nodes\n", toremove));
                            removebarrierdeliver = 1;
                            csize = count_consumers();
                            //VB3(("[mon] --- to wait worker size == consumer size\n"))
                            while(workersize != csize){
                                //VB3(("[mon] waiting for workersize %d == consumer queue size %d\n", workersize, csize));
                                //sleep(1); 
                                csize = count_consumers();
                            }
                            //VB3(("[mon] --- to wait worker size == consumer size OK\n"))
                            removebarrierresponse = 1;
                            //VB3(("[mon] finished waiting for workersize %d == consumer queue size %d\n", workersize, csize));
                            del_many_consumers(toremove);
                            //for (i = 0; i < toremove; i++){
                                //VB3(("[mon] removing %d out of %d\n", i+1, toremove))
                                //del_consumer(nodes.grank_size-1);
                                //sleep(2);
                            //}
                            //correct nodes after remove
                            //struct NodeConsumer * current;
                            //current = head_consumer;
                            //while (current){
                            //    current->id = current->id-1;
                            //    current = current->next;
                            //}
                            removebarrierresponse = 0;
                            removebarrierdeliver = 0;
                            //VB3(("[mon] monstate:Removing %d nodes OK\n", toremove));
                        }
                        //del_consumer(nodes.grank_size-1);
                    //}
                }
            }
        }else{
            //VB3(("[mon] -- reqsize 0 or workersize 0\n"));
            if (workersize > 1){
                //XXX toremove = nodes.grank_size-3;
                toremove = numprocs-3;
                //VB3(("[mon] monstate:2case-Reqsize empty. Removing %d nodes\n", toremove));
                if (toremove > 0)
                    del_many_consumers(toremove);
                //VB3(("[mon] monstate:2case-Reqsize empty. Removing %d nodes OK\n", toremove));
            }
        }
        //if (reqsize+11 > 10){
        //    VB3(("Add consumer\n"));
        //    add_consumer(); 
        //}
        //add_consumer();
        //dump_nodes(&nodes);
        sleep(5);
    }
}

void *controller_deliver(void *arg){
    //VB3(("[deliver] THREAD Controller Deliver UP\n"));
    MPI_Request treq;
    struct NodeConsumer * current;
    struct Node * node;
    while (1){
        if (head_request){
            while (removebarrierdeliver == 1){
                //sleep(1);
            }
            //current = head_request;
            //VB3(("[deliver] Deliver wait consumer_avail\n"));
            sem_wait(&consumer_avail);
            //VB3(("[deliver] Deliver wait consumer_avail PASSED\n"));
            current = head_consumer;
            if (current == NULL){
                //VB3(("[deliver] No consumer available\n"));
                continue;
            }
            while (current){
                //VB3(("[deliver] --> req %d\n", current->id));
                current = current->next;
            }
            //VB3(("[deliver] Deliver wait consumer_avail OK.\n"));
            //VB3(("[deliver] To perform fetch_node_from_gr(%d).\n", head_consumer->id));
            node = fetch_node_from_gr(head_consumer->id);
            //VB3(("[deliver] deliver #reqid %d to global_rank %d %s\n", 
                  //      head_request->id,
                    //    head_consumer->id, 
                      //  node->hostname));
            MPI_Send(&(head_request->id),
                      1,
                      MPI_INT,
                      head_consumer->id,
                      50,
                      //XXX mcmpi_comm
                      MPI_COMM_WORLD 
                      //*node->comm,
                      );
            consumer_del(head_consumer->id);
            req_del(head_request->id);
        }
    }
}

void *controller_response(void *arg){
    VB3(("[resp] THREAD Controller Response UP\n"));

    int flag, src, tag;
    int idx;
    MPI_Status probestatus;
    MPI_Request treq;
    while (1){
        flag = 0;
        while (!flag){
            while (removebarrierresponse == 1){
                //VB3(("[resp] is removing\n"));
                //sleep(1);
            }
            //for (idx = 2; idx < nodes.size; idx++){
                //VB(("response iprobe %s\n", nodes.hostname[idx]));
                //VB3(("wait Iprobe controlle response\n"));
                MPI_Iprobe(MPI_ANY_SOURCE,
                           51,
                           //nodes.mpicomm[idx],
                           //XXX mcmpi_comm,
                           MPI_COMM_WORLD,
                           &flag,
                           &probestatus);
                //VB3(("after wait Iprobe\n"));
                if (flag) break;
            //}
        }
        tag = probestatus.MPI_TAG;
        src = probestatus.MPI_SOURCE;
        int reqid;
        //VB3(("[resp] To receive response %d from %d\n", reqid, src));
        MPI_NRecv(&reqid,
                 1,
                 MPI_INT,
                 src,
                 tag,
                 //nodes.mpicomm[idx],
                 //XXX mcmpi_comm,
                 MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE
                );
        //VB3(("[resp] Received response %d from %d\n", reqid, src));
        //if (reqid == -1){
        //    consumer_push(src);
        //    sem_post(&consumer_avail);
        //    continue;
        //}
        consumer_push(src);
        sem_post(&consumer_avail);
        MPI_Isend(&reqid,
                  1,
                  MPI_INT,
                  0, //client
                  50,
                  //nodes.mpicomm[1],
                  //mcmpi_comm,
                  client_comm,
                  &treq
                );
        //VB3(("consumer_push %d\n", nodes.global_rank[idx]));
        //consumer_push(nodes.global_rank[idx]);
    }
}

void *controller_gateway(void *arg){
    VB3(("[gtw] THREAD Controller Gateway UP\n"));

    int flag, tag, src, idx;
    MPI_Status probestatus;
    //WWW excluido dump_nodes(&nodes);
    while (1){
        //VB3(("[gtw] Received gateway wait iprobe\n"));
        flag = 0;
//WWW excluido
        while (!flag){
/*            while (isremoving == 1){
                //VB3(("[gtw] is removing\n"));
                //sleep(1);
            }
*/
            //VB3(("wait MPI_NIprobe\n"));
            MPI_NIprobe(MPI_ANY_SOURCE,
                       50,
                       //nodes.mpicomm[1], // client
                       //mcmpi_comm,
                       client_comm,
                       &flag,
                       &probestatus);
            if (flag) break;
        }
        //VB3(("[gtw] Received gateway iprobe\n"));
        tag = probestatus.MPI_TAG;
        src = probestatus.MPI_SOURCE;
        int reqid;
//WWW excluido
/*
        while (isremoving == 1){
            //VB3(("[gtw] is removing\n"));
            //sleep(1);
        }
*/
        MPI_NRecv(&reqid,
                 1,
                 MPI_INT,
                 src,
                 tag,
                 //nodes.mpicomm[1],
                 //mcmpi_comm,
                 client_comm,
                 MPI_STATUS_IGNORE
                );
        //VB3(("[gtw] Received reqid %d\n", reqid));
        int start_ts = (int)time(NULL);
        req_push(reqid, start_ts);
    }
}

void service_work(){
    VB3(("[svc] TODO service work\n"));

    int a = -1;
    //if (global_rank != 1){
    //    VB3(("[svc] send -1 to controller\n"));
    //    MPI_NSend(&a, 1, MPI_INT, 0, 51, mcmpi_comm);
    //    VB3(("[svc] send -1 to controller ok\n"));
    //}

    int flag, tag, src;
    MPI_Status probestatus;
    while(1){
        //sem_post(&worker_wait_thread);
        //sem_wait(&worker_wait);
        flag = 0;
        while (!flag) {
//WWW excluido
/*
            while (isremoving == 1){
                //VB3(("[svc] is removing\n"));
                //sleep(1);
            }
*/
            MPI_Iprobe(MPI_ANY_SOURCE,
                       50,
            //XXX           mcmpi_comm,
                       MPI_COMM_WORLD,
                       &flag,
                       &probestatus);
        }
        tag = probestatus.MPI_TAG;
        src = probestatus.MPI_SOURCE;
        VB3(("[svc] Service received probe task\n"));
        MPI_NRecv(&a, 
                 1, 
                 MPI_INT, 
                 src,
                 tag, 
        //XXX         mcmpi_comm, 
                 MPI_COMM_WORLD, 
                 MPI_STATUS_IGNORE);
        VB3(("[svc] Worker> Received task %d from %d\n", a, src));
        sleep(3);
        VB3(("[svc] Worker> Reply task %d\n", a));
        MPI_NSend(&a, 1, MPI_INT, src, 51, MPI_COMM_WORLD);
        VB3(("[svc] Worker> Reply send ok %d\n", a));
        //sem_post(&worker_wait_thread);
        //sem_post(&worker_wait);
    }


}



char ** spawn_ec2(int n){
    //VB3(("Spawning %d ec2\n", n));
    //XXX char **servers = (char*) malloc(sizeof(char*)*n);
    char **servers;
    int i;
    servers = (char**) malloc(sizeof(char*)*n);
    for (i = 0; i < n; i++){
        servers[i] = (char*) malloc(sizeof(char) * 1000);
    }

    char cmd[100];
    sprintf(cmd, "python3 spawn-aws.py %d", n);
    FILE * fp = popen(cmd, "r");
    char * line = malloc(sizeof(char)*1000);
    for (i = 0; i < n; i++){
        line = fgets(line, 1000, fp);
        line = rtrim(line);
        strcpy(servers[i], line);
        //VB3(("server started[%d]  %s\n", i, servers[i]));
    }
    //fgets(line, 1000, fp); 
    //line = rtrim(line);
    return servers;
}




void req_del(int id){
    sem_wait(&req_lock);
    struct NodeReq * current;
    struct NodeReq * prev;
    prev = NULL;
    current = head_request;
    while (current){
        if (current->id == id){ // delete
            if (prev == NULL){
                if (tail_request == current)
                    tail_request = current->next;
                head_request = current->next;
            }else{
                if (current == tail_request){
                    tail_request = prev;
                }
                prev->next = current->next;
            }
            free(current);
            //VB3(("Req %d deleted\n", id));
            reqsize--;
            break;
        } // end if
        current = current->next;
    }
    sem_post(&req_lock);
}

void req_push(int id, int start_ts){
    // action{action, type, size, tag, dest, buffer_id, src}
    //VB3(("Request queued %d %d\n", id, start_ts));
    struct NodeReq *n = malloc(sizeof(struct NodeReq));
    n->id = id;
    n->start_ts = start_ts;
    n->next = NULL;

    sem_wait(&req_lock);
    if (head_request == NULL){
        head_request = n;
        tail_request = head_request;
    }else{
        tail_request->next = n;
        tail_request = tail_request->next;
    } 
    reqsize++;
    sem_post(&req_lock);
}

void consumer_del(int id){
    sem_wait(&consumer_lock);
    struct NodeConsumer * current;
    struct NodeConsumer * prev;
    prev = NULL;
    current = head_consumer;
    while (current){
        if (current->id == id){ // delete
            if (prev == NULL){
                if (tail_consumer == current)
                    tail_consumer = current->next;
                head_consumer = current->next;
            }else{
                if (current == tail_consumer){
                    tail_consumer = prev;
                }
                prev->next = current->next;
            }
            free(current);
            consumersize--;
            //VB3(("Consumer %d deleted\n", id));
            break;
        } // end if
        prev = current;
        current = current->next;
    }
    sem_post(&consumer_lock);
}

void consumer_push(int id){
    // action{action, type, size, tag, dest, buffer_id, src}
    struct NodeConsumer * current;
    current = head_consumer;
    while (current){
        if (current->id == id) return;
        current = current->next;
    }
    // wont add if already exists duplicates

    //VB3(("Consumer added %d\n", id));
    struct NodeConsumer * n = malloc(sizeof(struct NodeConsumer));
    n->id = id;
    n->next = NULL;

    sem_wait(&consumer_lock);
    if (head_consumer == NULL){
        head_consumer = n;
        tail_consumer = head_consumer;
    }else{
        tail_consumer->next = n;
        tail_consumer = tail_consumer->next;
    } 
    consumersize++;
    sem_post(&consumer_lock);
}

//void add_consumer(){
//    char * srv = spawn_ec2();
//    int grank = mcmpi_add_node(srv);
//    consumer_push(grank);
//    sem_post(&consumer_avail);
//    dump_nodes(&nodes);
//}

void add_consumer(int n){
    //char ** servers = spawn_self(n);
    //char ** servers = spawn_ec2(n);
    char ** servers = spawn_container(n);
    int i;
    for (i = 0; i < n; i++){
        //VB3(("add_consumer: to spawn %s\n", servers[i]));
        //VB3(("  [mon] monstate:Spawning %s\n", servers[i]));
        int grank = mcmpi_add_node(servers[i]);
        numprocs=numprocs+1;  //YYY incluido
        //VB3(("  [mon] monstate:Spawning %s OK\n", servers[i]));
        sleep(1);
        consumer_push(grank);
        sem_post(&consumer_avail);
        //WWW excluido dump_nodes2(&nodes);
    }
}

void del_many_consumers(int n){
    //XXX int lower = nodes.grank_size-n;
    int lower = numprocs-n;
    //int upper = nodes.grank_size;
    int upper = numprocs;
    mcmpi_remove_nranks(n);
    numprocs=numprocs-n;  //YYY incluido
    char cmd[100];
    sprintf(cmd, "python3 spawn-aws.py -%d", n);
    FILE * fp = popen(cmd, "r");

    //app
    //VB3(("[del_consumer] before consumer_del\n"));
    print_consumers();
    int i;
    for (i = lower; i < upper; i++){
        //VB3(("[del_consumer] to consumer_del %d\n", i));
        consumer_del(i);
    }
    //VB3(("[del_consumer] after consumer_del\n"));
    print_consumers();
    //VB3(("[del_consumer] wait consumer_avail\n"));
    sem_wait(&consumer_avail);
    //VB3(("[del_consumer] done wait consumer_avail\n"));
    //WWW excluido dump_nodes2(&nodes);
}

void del_consumer(int n){
    mcmpi_remove_rank(n);
    char cmd[100];
    sprintf(cmd, "python3 spawn-aws.py -1");
    FILE * fp = popen(cmd, "r");

    //app
    //VB3(("[del_consumer] before consumer_del\n"));
    print_consumers();
    consumer_del(n);
    //VB3(("[del_consumer] after consumer_del\n"));
    print_consumers();
    //VB3(("[del_consumer] wait consumer_avail\n"));
    sem_wait(&consumer_avail);
    //VB3(("[del_consumer] done wait consumer_avail\n"));
//    dump_nodes2(&nodes);
}

void print_consumers(){
    struct NodeConsumer * current;
    current = head_consumer;
    if (current == NULL){
        //VB3(("[print_consumers] is empty\n"));
        return;
    }
    while (current){
        //VB3(("[print_consumers] --> %d\n", current->id));
        current = current->next;
    }
}

int count_consumers(){
    struct NodeConsumer * current;
    int resp = 0;
    current = head_consumer;
    while (current){
        current = current->next;
        resp++;
    }
    return resp;
}




//////////////////////////////////////////////
//////////////////////////////////////////////
//////////////////////////////////////////////
//////////////////////////////////////////////

char *rtrim(char *s){
    char * back = s + strlen(s);
    while(isspace(*--back));
        *(back+1) = '\0';
    return s;
}

char ** spawn_self(int n){
   //VB3(("spawning %d containers\n", n)); 
   //XXX char **servers = (char*) malloc(sizeof(char*)*n);
   char **servers = (char**) malloc(sizeof(char*)*n);
   int i;
   for (i = 0; i < n; i++){
        servers[i] = (char*) malloc(sizeof(char)*100);
        strcpy("localhost", servers[i]);
   }
}

char ** spawn_container(int n){
   //VB3(("spawning %d containers\n", n)); 
   //XXX char **servers = (char*) malloc(sizeof(char*)*n);
   char **servers = (char**) malloc(sizeof(char*)*n);
   int i;
   for (i = 0; i < n; i++){
        servers[i] = (char*) malloc(sizeof(char)*100);
   }

   char cmd[100];
   sprintf(cmd, "python3 spawn-container.py %d", n);
   FILE * fp = popen(cmd, "r");
   char * line = malloc(sizeof(char)*1000);
   for (i = 0; i < n; i++){
        line = fgets(line, 1000, fp);
        line = rtrim(line);
        strcpy(servers[i], line);
        //VB3(("server started[%d] %s\n", i, servers[i]));
   }
   return servers;
}

//void add_consumer(int n){
//    char ** servers = spawn_ec2(n); 
//    int i;
//    for(i = 0; i < n; i++){
//        VB3(("add_consumer to spawn: %s\n", servers[i]));
//        mcmpi_add_node(servers[i]);
//        //VB3(("consumer up %s grank %d\n", servers[i], grank));
//        dump_nodes(&nodes);
//    }
//}
int main (int argc, char *argv[])
{
	MPI_Status status;
        // XXX int rank,numtasks;
	MPI_Init(&argc,&argv);
    printf("MPI_INIT FINISH\n\n"); fflush(0);
	MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
	MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
    int name_len;
    MPI_Get_processor_name(hostname, &name_len);
    printf("hello from %s rank %d\n", hostname, myrank); fflush(0);

//ate linha 469 de preload.c refere-se a MPI_Init
//app inicia em 471
	if (myrank==0){
        printf("controller going to userland %d\n", myrank); fflush(0);
        sem_post(&req_lock);
        sem_post(&consumer_lock);
        sem_post(&consumer_avail);
        printf("2\n"); fflush(0);
        consumer_push(1); // hardcoded for first worker
        printf("3\n"); fflush(0);
        controller_work();
	}
	if (myrank > 0){
		if (myrank == 1){
            printf("to start client_work\n"); fflush(0);
			client_work();
		}
		else {
      			//  VB(("to enter service_work\n"));
                printf("to start service_work\n"); fflush(0);
        		service_work();
    		}
	}


   MPI_Finalize();
}

