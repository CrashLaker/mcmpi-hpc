



void init_comm_buffers(){
    if (DEBUG) printf("init comm buffers\n");
    if (DEBUG) fflush(stdout);
    sem_init(full1,1,0);
    sem_init(empty1,1,size_buffer);
    sem_init(excl1,1,1);

    sem_init(full2,1,0);
    sem_init(empty2,1,size_buffer);
    sem_init(excl2,1,1);

    *pointer1=0;
    *count1=0;
    *pointer2=0;
    *count2=0;

}

char * create_comm_buffers2(){
    int id_buffer1;
    int id_excl1;
    int id_full1;
    int id_empty1;
    int id_pointer1;
    int id_count1;

    int id_buffer2;
    int id_excl2;
    int id_full2;
    int id_empty2;
    int id_pointer2;
    int id_count2;

    int id_cltnodetable; // cluster nodetable
    pid_t pid;
    if (DEBUG) printf("INICIO\n");
    if (DEBUG) fflush(stdout);

    id_buffer1=shmget (IPC_PRIVATE, sizeof(int)*size_buffer, 0777 | IPC_CREAT);
    id_excl1=shmget (IPC_PRIVATE, sizeof(sem_t), 0777 | IPC_CREAT);
    id_full1=shmget (IPC_PRIVATE, sizeof(sem_t), 0777 | IPC_CREAT);
    id_empty1=shmget (IPC_PRIVATE, sizeof(sem_t), 0777 | IPC_CREAT);
    id_pointer1=shmget (IPC_PRIVATE, sizeof(int), 0777 | IPC_CREAT);
    id_count1=shmget (IPC_PRIVATE, sizeof(int ), 0777 | IPC_CREAT);

    id_buffer2=shmget (IPC_PRIVATE, sizeof(int)*size_buffer, 0777 | IPC_CREAT);
    id_excl2=shmget (IPC_PRIVATE, sizeof(sem_t), 0777 | IPC_CREAT);
    id_full2=shmget (IPC_PRIVATE, sizeof(sem_t), 0777 | IPC_CREAT);
    id_empty2=shmget (IPC_PRIVATE, sizeof(sem_t), 0777 | IPC_CREAT);
    id_pointer2=shmget (IPC_PRIVATE, sizeof(int), 0777 | IPC_CREAT);
    id_count2=shmget (IPC_PRIVATE, sizeof(int), 0777 | IPC_CREAT);

    id_cltnodetable=shmget (IPC_PRIVATE, sizeof(nodes), 0777 | IPC_CREAT);

    buffer1 = shmat (id_buffer1, 0, 0);
    excl1 = shmat (id_excl1, 0, 0);
    full1 = shmat (id_full1, 0, 0);
    empty1 = shmat (id_empty1, 0, 0);
    pointer1 = shmat (id_pointer1, 0, 0);
    count1 = shmat (id_count1, 0, 0);

    buffer2 = shmat (id_buffer2, 0, 0);
    excl2 = shmat (id_excl2, 0, 0);
    full2 = shmat (id_full2, 0, 0);
    empty2 = shmat (id_empty2, 0, 0);
    pointer2 = shmat (id_pointer2, 0, 0);
    count2 = shmat (id_count2, 0, 0);

    cltnodes = shmat(id_cltnodetable, 0, 0);

    char buffer1str[1000];
    char buffer2str[1000];
    sprintf(buffer1str, "%d/%d/%d/%d/%d/%d", id_buffer1, id_excl1, id_full1, id_empty1, id_pointer1, id_count1);
    sprintf(buffer2str, "%d/%d/%d/%d/%d/%d", id_buffer2, id_excl2, id_full2, id_empty2, id_pointer2, id_count2);
    char * b2str = (char *) malloc(sizeof(char)*1000);
    sprintf(b2str, "%s/%s/%d", buffer2str, buffer1str, id_cltnodetable);
    char filetosave[200];
    return b2str;
}


void create_comm_buffers(char * hostname){
    int id_buffer1;
    int id_excl1;
    int id_full1;
    int id_empty1;
    int id_pointer1;
    int id_count1;

    int id_buffer2;
    int id_excl2;
    int id_full2;
    int id_empty2;
    int id_pointer2;
    int id_count2;
    pid_t pid;

    id_buffer1=shmget (IPC_PRIVATE, sizeof(char)*size_buffer, 0777 | IPC_CREAT);
    id_excl1=shmget (IPC_PRIVATE, sizeof(sem_t), 0777 | IPC_CREAT);
    id_full1=shmget (IPC_PRIVATE, sizeof(sem_t), 0777 | IPC_CREAT);
    id_empty1=shmget (IPC_PRIVATE, sizeof(sem_t), 0777 | IPC_CREAT);
    id_pointer1=shmget (IPC_PRIVATE, sizeof(int), 0777 | IPC_CREAT);
    id_count1=shmget (IPC_PRIVATE, sizeof(int ), 0777 | IPC_CREAT);

    id_buffer2=shmget (IPC_PRIVATE, sizeof(char)*size_buffer, 0777 | IPC_CREAT);
    id_excl2=shmget (IPC_PRIVATE, sizeof(sem_t), 0777 | IPC_CREAT);
    id_full2=shmget (IPC_PRIVATE, sizeof(sem_t), 0777 | IPC_CREAT);
    id_empty2=shmget (IPC_PRIVATE, sizeof(sem_t), 0777 | IPC_CREAT);
    id_pointer2=shmget (IPC_PRIVATE, sizeof(int), 0777 | IPC_CREAT);
    id_count2=shmget (IPC_PRIVATE, sizeof(int ), 0777 | IPC_CREAT);

    buffer1 = shmat (id_buffer1, 0, 0);
    excl1 = shmat (id_excl1, 0, 0);
    full1 = shmat (id_full1, 0, 0);
    empty1 = shmat (id_empty1, 0, 0);
    pointer1 = shmat (id_pointer1, 0, 0);
    count1 = shmat (id_count1, 0, 0);

    buffer2 = shmat (id_buffer2, 0, 0);
    excl2 = shmat (id_excl2, 0, 0);
    full2 = shmat (id_full2, 0, 0);
    empty2 = shmat (id_empty2, 0, 0);
    pointer2 = shmat (id_pointer2, 0, 0);
    count2 = shmat (id_count2, 0, 0);

    char buffer1str[1000];
    char buffer2str[1000];
    sprintf(buffer1str, "%d/%d/%d/%d/%d/%d", id_buffer1, id_excl1, id_full1, id_empty1, id_pointer1, id_count1);
    sprintf(buffer2str, "%d/%d/%d/%d/%d/%d", id_buffer2, id_excl2, id_full2, id_empty2, id_pointer2, id_count2);
    char b2str[1000];
    sprintf(b2str, "%s/%s", buffer2str, buffer1str);
    char filetosave[200];
    sprintf(filetosave, "%s-buffers", hostname);
    save_2file(filetosave, b2str);
}

void parse_cluster_resp(char * resp, int * global_rank, int * total_proc, int * tgrank, char * command){
    char procstring[10];
    int i;
    for (i = 0; i < 10; i++) procstring[i] = '\0';
    i = 1;
    int index = 0;
    int iter = 0;
    char * tostring = procstring;
    while (resp[i] != '\0'){
        if (resp[i] == '/'){
            tostring[index] = '\0';
            if (iter == 0){
                *global_rank = atoi(tostring);
                iter += 1;
                i++;
                index = 0;
                tostring[0] = '\0';
                continue;
            }else if (iter == 1){
                *tgrank = atoi(tostring);
            }
            tostring = command;
            index = 0;
            i++;
            continue;
        }
        tostring[index] = resp[i];
        index++;
        i++;
    }
    tostring[index] = '\0';
    *total_proc = atoi(procstring);
    return;
}

void create_comm_buffers_from_env(){
    char * command = getenv("MCMPI_BUFFERS");
    int * rs = parse_buffer_command(command);
    buffer1 = shmat (rs[0], 0, 0);
    excl1 = shmat (rs[1], 0, 0);
    full1 = shmat (rs[2], 0, 0);
    empty1 = shmat (rs[3], 0, 0);
    pointer1 = shmat (rs[4], 0, 0);
    count1 = shmat (rs[5], 0, 0);

    buffer2 = shmat (rs[6], 0, 0);
    VB((">>>>>>>>>>>>>>>>>>>>>>>> buffer2 %p\n", buffer2));
    excl2 = shmat (rs[7], 0, 0);
    full2 = shmat (rs[8], 0, 0);
    empty2 = shmat (rs[9], 0, 0);
    pointer2 = shmat (rs[10], 0, 0);
    count2 = shmat (rs[11], 0, 0);

    cltnodes = shmat(rs[12], 0, 0);
}

void create_comm_buffers_from_file(char * hostname){
    char filename[200];
    sprintf(filename, "%s-buffers", hostname);
    char * command = load_file(filename);
    int * rs = parse_buffer_command(command);
    buffer1 = shmat (rs[0], 0, 0);
    excl1 = shmat (rs[1], 0, 0);
    full1 = shmat (rs[2], 0, 0);
    empty1 = shmat (rs[3], 0, 0);
    pointer1 = shmat (rs[4], 0, 0);
    count1 = shmat (rs[5], 0, 0);

    buffer2 = shmat (rs[6], 0, 0);
    excl2 = shmat (rs[7], 0, 0);
    full2 = shmat (rs[8], 0, 0);
    empty2 = shmat (rs[9], 0, 0);
    pointer2 = shmat (rs[10], 0, 0);
    count2 = shmat (rs[11], 0, 0);
}

void escreve(int carac){
    sem_wait(empty2);
    sem_wait(excl2);
    buffer1[(*pointer2+*count2)%size_buffer]=carac;
    *count2=*count2+1;

    sem_post(excl2);
    sem_post(full2);
    int val0, val1, val2, val3, val4;
    sem_getvalue(empty2, &val0);
    sem_getvalue(excl2, &val1);
    sem_getvalue(full2, &val2);
}
int leia(){
    int x;
    sem_wait(full1);
    sem_wait(excl1);

    x=buffer2[*pointer1];

    *count1=*count1-1;
    *pointer1=(*pointer1+1)%size_buffer;

    sem_post(excl1);
    sem_post(empty1);
    return x;
}

char * genmessage(int action, 
                  int type, 
                  int size, 
                  int tag, 
                  int dest, 
                  int buffer_id){
    char * msg = (char*) malloc(sizeof(char)*500);
    sprintf(msg, "%d/%d/%d/%d/%d/%d", action, type, size, tag, dest, buffer_id);
    return msg;
}

void send_command_arg(int action, 
                      int type_size, 
                      int size, 
                      int tag, 
                      int dest, 
                      int buffer_id,
                      int src){
    pthread_mutex_lock(send_command_arg_lock);
    escreve(action);
    escreve(type_size);
    escreve(size);
    escreve(tag);
    escreve(dest);
    escreve(buffer_id);
    escreve(src);
    pthread_mutex_unlock(send_command_arg_lock);
}

void produz(){
    char k = 'a';
    int * action = (int*) malloc(sizeof(int)*6);
    int flag;

    while(1){
        if (mcmpi_cluster_gateway == 1){
            while (barrier_progress == 1){
                sleep(1);
            }
        }
        flag = 0;
        MPI_Status status;
        flag = 0;
        while (!flag){
            MPI_Iprobe(MPI_ANY_SOURCE, 100, mcmpi_comm, &flag, &status);
        }
        int status_src = status.MPI_SOURCE;
        MPI_Recv(action, 6, MPI_INT, status_src, 100, mcmpi_comm, MPI_STATUS_IGNORE);
        int msg_action    = action[0];
        int msg_type_size = action[1];
        int msg_size      = action[2];
        int msg_tag       = action[3];
        int msg_src       = action[4];
        int msg_dest      = action[5];
        int cmd0    = action[0];
        int cmd1 = action[1];
        int cmd2      = action[2];
        int cmd3       = action[3];
        int cmd4       = action[4];
        int cmd5      = action[5];
        char msg[200];
        if (msg_action == 0){
            int msg_size_bytes = msg_size*msg_type_size;
            int id_buff = shmget(IPC_PRIVATE, msg_size_bytes, 0777 | IPC_CREAT);
            void * store_buff = shmat(id_buff, 0, 0);
            MPI_Recv(store_buff, 
                     msg_size_bytes, 
                     MPI_CHAR, 
                     status_src, 
                     100, 
                     mcmpi_comm,
                     MPI_STATUS_IGNORE);
            send_command_arg(0,
                             msg_type_size, 
                             msg_size, 
                             msg_tag, 
                             msg_src,
                             msg_dest, 
                             id_buff);
        }else if (msg_action == 1){ // Barrier message
            if (mcmpi_cluster_gateway == 1 && init == 1){
                send_command_arg(1, 0, 0, 0, 0, 0, 0);
                MPI_Barrier(mcmpi_comm);
            }else{
                barrier_progress = 1;
                send_command_arg(1, 0, 0, 0, 0, 0, 0);
            }
        }else if (msg_action == 3){
            init = 0;
            send_command_arg(msg_action, 0, 0, 0, 0, 0, 0);
        }else if (msg_action == 5){
            rebuild_node_table();
            send_command_arg(msg_action, 0, 0, 0, 0, 0, 0);
        }else if (msg_action == 15){
            int ttval[1000];
            sem_wait(&clt_barrier_lock);
            clt_barrier_ongoing_size++;
            if (mcmpi_cluster_gateway == 1){
                int limit = mysize-1;
                int gateway_nodes_size = count_gateway_nodes();
                if (gateway_nodes_size > 1)
                    limit = mysize - gateway_nodes_size + 1;
                if (clt_barrier_ongoing_size == limit){ // warn other gateways
                    int * buff = (int*) malloc(sizeof(int)*1000);
                    int i;
                    MPI_Request treq;
                    for (i = 0; i < nodes.size; i++){
                        if (strcmp(nodes.hostname[i], processor_name) != 0 &&
                            nodes.type[i] == 1){ // send to all gateways
                            if (1){
                                int action[6] = {15,0,0,0,0,0};
                                MPI_Isend(action, 
                                          6, 
                                          MPI_INT, 
                                          nodes.local_rank[i], 
                                          100, 
                                          mcmpi_comm, 
                                          &treq);
                            }else{
                                MPI_Isend(buff, 
                                          1000, 
                                          MPI_INT, 
                                          nodes.local_rank[i], 
                                          15, 
                                          mcmpi_comm, 
                                          &treq);
                            }
                        }
                    }
                }else if (clt_barrier_ongoing_size == mysize){ 
                    MPI_Barrier(mcmpi_comm);
                    send_command_arg(16,0,0,0,0,0,0);
                    clt_barrier_ongoing_size = 0;
                }
            }else if (mcmpi_cluster_gateway == 2){
                if (clt_barrier_ongoing_size == mysize-1){
                    send_command_arg(15,0,0,0,0,0,0);
                }
            }
            sem_post(&clt_barrier_lock);
        }else if (msg_action == 22){ // call from controller mcmpi_remove_rank
            _mcmpi_remove_rank(cmd1, cmd2);
        }else if (msg_action == 30){ // mcmpi_scale_on_val ON
            if (mcmpi_scale_on_val == 0){
                mcmpi_scale_on_val = 1;
                if (pthread_create(&tid_thread_work, NULL, thread_work, NULL)){
                    VB(("Cannot create thread\n"));
                }
            }
        }else if (msg_action == 31){ // mcmpi_scale_on_val = OFF
            mcmpi_scale_on_val = 0;
        }else if (msg_action == 101){
            send_command_arg(8, -1, cmd1, 0, 0, 0, 0); 
        }
    }
}

void produz2(){
    char k = 'C';
    while(1){
        escreve(k);
        sleep(1);
    }
}

void exec_command(int * cmd){
    MPI_Request req;
    if (cmd[0] == 0){
        struct shmid_ds * bufdel;
        skip_route = 1;
        int msg_type_size = cmd[1];
        int msg_size      = cmd[2];
        int msg_tag       = cmd[3];
        int msg_src       = cmd[4];
        int msg_dest      = cmd[5];
        int msg_id_buffer = cmd[6];
        int msg_size_bytes = msg_type_size * msg_size;
        void * rec_msg = shmat(msg_id_buffer, 0, 0);
        struct Node * my_node = fetch_node_from_hostname(processor_name);
        struct Node * node_to = fetch_node_from_gr(msg_dest);
        show_node_info2("My_Node", my_node);
        show_node_info2("Node_to", node_to);
        if (init == 1 || strcmp(node_to->comm, "global") == 0 || 
                strcmp(node_to->comm, my_node->comm) == 0 ||
                strcmp(node_to->comm, my_node->hostname) == 0){
            MPI_Request * req = malloc(sizeof(MPI_Request));
            MPI_Status status;
            MPI_Isend(rec_msg, 
                      msg_size_bytes, 
                      MPI_CHAR, 
                      node_to->local_rank, 
                      msg_tag, 
                      mcmpi_comm, 
                      req);
            task_push(msg_id_buffer, req, msg_src, msg_dest, msg_tag);
        }else{
            struct Node * node_gateway = fetch_gateway_comm_node(node_to->comm);
            show_node_info2("Node_gateway", node_gateway);
            int action[6] = {0,msg_type_size,msg_size,msg_tag,msg_src,msg_dest};
            show_action_info(action);
            MPI_Send(action, 6, MPI_INT, node_gateway->local_rank, 100, mcmpi_comm);
            MPI_Send(rec_msg, 
                     msg_size_bytes, 
                     MPI_CHAR, 
                     node_gateway->local_rank, 
                     100, 
                     mcmpi_comm);
        }
    }else if (cmd[0] == 1){ // Barrier
        if (mcmpi_cluster_gateway == 2 && init == 1){
            sem_post(cluster_barrier);
        }else if (mcmpi_cluster_gateway == 2 && init == 0){
            MPI_Barrier(mcmpi_comm);
            send_command_arg(1, 0, 0, 0, 0, 0, 0);
        }else if (mcmpi_cluster_gateway == 1 && init == 0){
            MPI_Barrier(mcmpi_comm);
            barrier_progress = 0;
        }
    }else if (cmd[0] == 2){ // GRank
        grank_start = cmd[1];
    }else if (cmd[0] == 3){ // Disable init
        init = 0;
    }else if (cmd[0] == 4){
        struct shmid_ds * bufdel;
        shmctl(cmd[1], IPC_RMID, bufdel);
    }else if (cmd[0] == 5){
        rebuild_node_table();
    }else if (cmd[0] == 6){
        global_rank = cmd[1];
    }else if (cmd[0] == 7){ // gateway1->gateway2
                            // notify cluster nodes that controller has
                            // finished reading mcmpi_hostfile
        int i;
        init = 0;
        for (i = 0; i < nodes.size; i++){
            if (strcmp(nodes.comm[i], processor_name) ==0){
                int tbuff[1000];
                MPI_Send(&tbuff, 
                         1000, 
                         MPI_INT, 
                         nodes.local_rank[i], 
                         7, 
                         mcmpi_comm);
            }
        }
    }else if (cmd[0] == 8){ // Isend task was completed successfully
        int id_buffer = cmd[1];
        int src = cmd[2];
        int tag = cmd[3];
        if (id_buffer != -1){ // when comm between gateways
            shmdt(id_buffer);
            shmctl(id_buffer, IPC_RMID, NULL);
        }
        struct Node * my_node = fetch_node_from_hostname(processor_name);
        struct Node * node_to = fetch_node_from_gr(src);
        show_node_info2("My_node", my_node);
        show_node_info2("Node_to", node_to);
        MPI_Request treq;
        if (strcmp(my_node->comm, node_to->comm) == 0){
            // VB(("cmd8.1 mpi_isend message ack back to %s %d %d\n", 
            //              node_to->hostname,
            //              node_to->local_rank,
            //              node_to->global_rank));
            MPI_Isend(&src,
                      1,
                      MPI_INT,
                      node_to->local_rank,
                      tag,
                      mcmpi_comm,
                      &treq);
        }else if (strcmp(my_node->hostname, node_to->comm) == 0){
            // belongs to same cluster
            // cluster1gateway -> cluster1node02 (part of cluster1gateway)
            // VB(("cmd8.2 mpi_isend message ack back to %s %d %d\n", 
            //              node_to->hostname,
            //              node_to->local_rank,
            //              node_to->global_rank));
            MPI_Isend(&src,
                      1,
                      MPI_INT,
                      node_to->local_rank,
                      tag,
                      mcmpi_comm,
                      &treq);
        }else{
            // sends between different clusters
            struct Node * node_gateway = fetch_gateway_comm_node(node_to->comm);
            // VB(("cmd 8.3 send ACT101 to gateway %s\n", node_gateway->hostname));
            if (1){
                int action[6] = {101,src,0,0,0,0};
                MPI_Isend(action,
                         6,
                         MPI_INT,
                         node_gateway->local_rank,
                         100,
                         mcmpi_comm,
                         &treq);
            }else{
                MPI_Send(&src,
                         1,
                         MPI_INT,
                         node_gateway->local_rank,
                         101,
                         mcmpi_comm);
            }
        }
    }else if (cmd[0] == 10){ // Send ACK Barrier command
        skip_route = 1;
        int * buff = (int*) malloc(sizeof(int)*1000);
        MPI_Send(buff, 1000, MPI_INT, 0, 0, mcmpi_comm);
    }else if (cmd[0] == 15){ // gateway1 send global barrier
        sem_wait(&clt_barrier_lock);
        clt_barrier_ongoing_size++;
        if (clt_barrier_ongoing_size == mysize-1){
            MPI_Request treq;
            int * buff = (int*) malloc(sizeof(int)*1000);
            int i;
            for (i = 0; i < nodes.size; i++){
                if (strcmp(nodes.hostname[i], processor_name) != 0 &&
                    nodes.type[i] == 1){ // send to all gateways
                    if (1){
                        int action[6] = {15,0,0,0,0,0};
                        MPI_Isend(action, 
                                  6, 
                                  MPI_INT, 
                                  nodes.local_rank[i], 
                                  100, 
                                  mcmpi_comm, 
                                  &treq);
                    }else{
                        MPI_Isend(buff, 
                                  1000, 
                                  MPI_INT, 
                                  nodes.local_rank[i], 
                                  15, 
                                  mcmpi_comm, 
                                  &treq);
                    }
                }
            }
        }else if (clt_barrier_ongoing_size == mysize){
            send_command_arg(16,0,0,0,0,0,0);
            clt_barrier_ongoing_size = 0;
            MPI_Barrier(mcmpi_comm); 
        }
        sem_post(&clt_barrier_lock);
    }else if (cmd[0] == 16){ // gateway1->gateway2 proceede barrier
        MPI_Barrier(mcmpi_comm);
    }else if (cmd[0] == 20){ // from gateway2 refresh cltnodes
        dump_nodes(cltnodes);
        MPI_Send(cltnodes, sizeof(nodes), MPI_CHAR, 0, 0, mcmpi_comm);
    }else if (cmd[0] == 21){ // refresh sync_table
        memcpy(&nodes, cltnodes, sizeof(nodes));
        dump_nodes(&nodes);
        int i;
        for (i = 0; i < nodes.size; i++){
            if (strcmp(nodes.comm[i], processor_name) != 0) continue;
            MPI_Request treq;
            MPI_Isend(&nodes, 
                    sizeof(nodes), 
                    MPI_CHAR, 
                    nodes.local_rank[i], 
                    21, 
                    mcmpi_comm,
                    &treq);
        }
    }
}
void consome(){
    char k;
    int i;
    int parsed[7] = {0,1,2,3,4,5,6};
    // 0 action
    // 1 type 
    // 2 size 
    // 3 tag  
    // 4 src 
    // 5 dest 
    // 6 buffer_id 
    while (1){
        int sindex = 0;
        int i;
        if (1){
            for(i=0; i<7;i++)
                parsed[i] = leia();
        }else{
            char tempstring[1000];
            while ((k=leia())!='.'){
                //putchar(k);
                tempstring[sindex] = k;
                sindex++;
            }
            tempstring[sindex] = '\0';
        }
        int full1val;
        int full2val;
        sem_getvalue(full1, &full1val);
        sem_getvalue(full2, &full2val);
        exec_command(&parsed);
    }

}

int * parse_buffer_command(char * str){
    int * rs = (int*) malloc(sizeof(int)*20);
    int iter = 0;
    int i, j;
    char tempstring[100];
    int index_tempstr = 0;
    for (i = 0; i < 100; i++) tempstring[i] = '\0';
    int indexstr = 0;
    while (str[indexstr] != '\0'){
        if (str[indexstr] == '/'){
            rs[iter] = atoi(tempstring);
            for (i = 0; i < 100; i++) tempstring[i] = '\0';
            index_tempstr = 0;
            iter++;
            indexstr++;
        }
        tempstring[index_tempstr] = str[indexstr];
        index_tempstr++;
        indexstr++;
    }
    rs[iter] = atoi(tempstring);
    iter++;
    return rs;
}

void *inout_thread(void * v){
    int *tid = (int*) v;
    if (*tid == 0){
        produz();
    }else{
        consome();
    }
}
void *inout_thread2(void * v){
    int *tid = (int*) v;
    if (*tid == 0){
        produz2();
    }else{
        consome();
    }
}
