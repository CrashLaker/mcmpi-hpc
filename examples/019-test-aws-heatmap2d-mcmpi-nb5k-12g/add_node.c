//           _     _                     _
//  __ _  __| | __| |    _ __   ___   __| | ___
// / _` |/ _` |/ _` |   | '_ \ / _ \ / _` |/ _ \
//| (_| | (_| | (_| |   | | | | (_) | (_| |  __/
// \__,_|\__,_|\__,_|___|_| |_|\___/ \__,_|\___|
//                 |_____|
//tag:add_node
int mcmpi_add_node(char * hostname_add){
    int grank = _mcmpi_add_node(hostname_add);
    int i_grank = get_grank_index(grank);
    mpi_send_f_type mpi_send;
    mpi_send = (mpi_send_f_type)dlsym(RTLD_NEXT,"MPI_Send");
    char a[5000] = "-"; // standalone
    mpi_send(a, 5000, MPI_CHAR, nodes.local_rank[i_grank], 0, mcmpi_comm);
    sync_table();
    return grank;
}
int* mcmpi_add_cluster(char * hostname_add, char * node_list){
    int grank = _mcmpi_add_node(hostname_add);
    int i_grank = get_grank_index(grank);
    nodes.size--;
    nodes.grank_size--;
    add_node(hostname_add, 1, nodes.local_rank[i_grank], -1, "global");
    mpi_send_f_type mpi_send;
    mpi_send = (mpi_send_f_type)dlsym(RTLD_NEXT,"MPI_Send");

    mpi_send(node_list, 5000, MPI_CHAR, nodes.local_rank[i_grank], 0, mcmpi_comm);
    mpi_send(&nodes.grank_size, 1, MPI_INT, nodes.local_rank[i_grank], 0, mcmpi_comm);
    int * a;
    int server_count = 0;
    char **servers_list = split_servers(node_list, &server_count); 
    struct Nodes clusterNodes;
    MPI_Recv(&clusterNodes, 
            sizeof(clusterNodes), 
            MPI_CHAR, 
            nodes.local_rank[i_grank], 
            0,
            mcmpi_comm, &status);
    dump_nodes(&clusterNodes);
    int i;
    for (i = 0; i < clusterNodes.size; i++){
        add_node(
            clusterNodes.hostname[i],
            clusterNodes.type[i],
            clusterNodes.local_rank[i],
            clusterNodes.global_rank[i],
            clusterNodes.comm[i]
        );
        nodes.grank_size++;
    }
    dump_nodes(&nodes);
    return &a;
}
int _mcmpi_add_node(char * hostname_add){
    MPI_Info_create(&minfo);
    MPI_Info_create(&info);
    MPI_Info_set(info, "localhost", "false");
    MPI_Info_set(info, "ompi_global_scope", "true");
    MPI_Open_port(MPI_INFO_NULL, port_name);
    MPI_Publish_name("controller", info, port_name);
    char hostnames[10][MAX_HOSTNAME_LEN];
    char return_hostname[MAX_HOSTNAME_LEN];
    int i;
    int nodes_to_create = 1;


    //MPI_Comm *clients_comm[nodes_to_create];
    //char portnames[nodes_to_create][MAX_HOSTNAME_LEN];
    MPI_Comm client_comm;
    char postname[MAX_HOSTNAME_LEN];
    mpi_send_f_type mpi_send;
    mpi_send = (mpi_send_f_type)dlsym(RTLD_NEXT,"MPI_Send");
    
    if (nodes.grank_size > 1){
        // Trigger already spawned nodes
        dump_nodes(&nodes);
        for (i = 1; i <= nodes.size-1; i++){
            if (strcmp(nodes.comm[i], "global") == 0 &&
                nodes.local_rank[i] != -1 &&
                nodes.local_rank[i] != 0){
                mpi_send(&i, 1, MPI_INT, nodes.local_rank[i], 20, mcmpi_comm);
            }
        }
    }else{
        mcmpi_comm = MPI_COMM_WORLD;
    }
    int node_id;
    char cmd[1000];
    char * mcmpi_server_uri = getenv("MCMPI_SERVER_URI");
    char * ldpreload = getenv("LD_PRELOAD");
    char * app_path = getenv("APP_PATH");
    sprintf(cmd, "ssh %s \"cd `pwd`; export LD_PRELOAD=%s; export MCMPI_APP=1; export MCMPI_PORT_NAME=%s; %s nohup mpirun --mca btl_tcp_if_include '192.168.16.0/20' --allow-run-as-root -n 1 --ompi-server \\\"%s\\\" %s 2>&1 | tee -a /mpilog \"&",
                         hostname_add, ldpreload, port_name, mcmpi_hostfile_flag, mcmpi_server_uri, app_path);
    system(cmd);

    MPI_Comm_accept(port_name, MPI_INFO_NULL, 0, mcmpi_comm, &intercomm);
    mycomms[nodes.size-1] = intercomm;
    MPI_Intercomm_merge(intercomm, 0, &mcmpi_comm);
    int created_node_local_rank;
    char created_node_hostname[MPI_MAX_PROCESSOR_NAME];
    MPI_NRecv(&created_node_local_rank, 1, MPI_INT, 0, 10, intercomm, &status);
    MPI_NRecv(created_node_hostname, 
             MPI_MAX_PROCESSOR_NAME, 
             MPI_CHAR, 
             0, 
             10, 
             intercomm, 
             &status);

    int grank = nodes.grank_size;

    add_node(created_node_hostname, 0, created_node_local_rank, grank, "global");

    MPI_NSend(&grank, 1, MPI_INT, 0 ,10, intercomm);

    nodes.grank_size++;
    return grank;
}

void children_stuff(){
    MPI_Comm intercomm = parentcomm; 
    
    int node_temp_index;
    int nodes_to_create;
    char * portname = getenv("MCMPI_PORT_NAME");
    strcpy(&parentportname, portname);
    MPI_Comm_connect(parentportname, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &parentcomm);
    MPI_Intercomm_merge(parentcomm, 1, &mcmpi_comm);
    MPI_Comm_rank(mcmpi_comm, &rank);
    MPI_Comm_size(mcmpi_comm, &size);

    wrld = mcmpi_comm;
    MPI_NSend(&rank, 1, MPI_INT, 0, 10, parentcomm);
    MPI_NSend(hostname, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, 0, 10, parentcomm);
    MPI_NRecv(&myrank, 1, MPI_INT, 0, 10, parentcomm, &status);
    global_rank = myrank;
    if (pthread_create(&tid, NULL, thread_work, NULL)){
        VB(("Cannot create thread\n"));
    }
    char node_list[5000];
    MPI_Recv(node_list, 5000, MPI_CHAR, 0, 0, mcmpi_comm, &status);
    if (node_list[0] != '-') { // cluster if not '-'
        int server_count = 0;
        char **servers_list = split_servers(node_list, &server_count); 
        mcmpi_cluster_gateway = 1;
        sem_post(&clt_barrier_lock); // start enabled
        sem_post(&task_loop_lock); 
        spawn_remote_mpi2(hostname, server_count, node_list);
    }
}

void *thread_work(void *arg){
    int newn;
    int flag;
    int tag;
    int src;
    int tval;
    MPI_Status probestatus;

    int codes[5] = {20,21,22,7,8};
    int code;
    int idx;
    while(1) {
        flag = 0;
        while (!flag){
            if (mcmpi_scale_on_val == 0) { // leave thread
                VB(("Leaving thread_work\n"));
                return;
            }

            for (idx = 0; idx < 5; idx++){
                MPI_Iprobe(
                    MPI_ANY_SOURCE, 
                    codes[idx],
                    mcmpi_comm, 
                    &flag, 
                    &probestatus
                );
                if (flag) break;
            }
            sleep(1);
        }
        tag = probestatus.MPI_TAG;
        src = probestatus.MPI_SOURCE;
        if (tag == 20) { // newn
            MPI_Recv(&newn, 1, MPI_INT, 0, 20, mcmpi_comm, MPI_STATUS_IGNORE);
            MPI_Comm_accept(parentportname, MPI_INFO_NULL, 0, wrld, &intercomm);
            MPI_Intercomm_merge(intercomm, 0, &mcmpi_comm);
            wrld=mcmpi_comm;
        }else if (tag == 21){ //refresh nodes struct table
            if (mcmpi_cluster_gateway != 0)
                MPI_Recv(&nodes, 
                         sizeof(nodes), 
                         MPI_CHAR, 
                         0, 
                         21, 
                         mcmpi_comm, 
                         MPI_STATUS_IGNORE);
            else
                MPI_XRecv(&nodes, 
                         sizeof(nodes), 
                         MPI_CHAR, 
                         0, 
                         21, 
                         mcmpi_comm, 
                         MPI_STATUS_IGNORE);
            MPI_Comm_size(mcmpi_comm, &mysize);
            global_size = mysize;
            dump_nodes(&nodes);
            if (mcmpi_cluster_gateway == 1){
                memcpy(cltnodes, &nodes, sizeof(nodes));
                send_command_arg(21, 0, 0, 0, 0, 0, 0);
            }
        }else if (tag == 22){ // remove rank
            isremoving = 1;
            int payload[2];
            if (mcmpi_cluster_gateway != 0)
                MPI_Recv(payload, 2, MPI_INT, src, 22, mcmpi_comm, MPI_STATUS_IGNORE);
            else
                MPI_XRecv(payload, 2, MPI_INT, src, 22, mcmpi_comm, MPI_STATUS_IGNORE);
            _mcmpi_remove_rank(payload[0], payload[1]);
            isremoving = 0;
        }else if (tag == 7){ // tbuff exit prologue
            if (mcmpi_cluster_gateway == 1){
                init = 0;
                int tbuff[1000];
                MPI_XRecv(&tbuff, 1000, MPI_INT, src, 7, mcmpi_comm, MPI_STATUS_IGNORE); 
                if (mcmpi_cluster_gateway == 1){
                    send_command_arg(7,0,0,0,0,0,0);
                }
            }else{
                int tbuff[1000];
                MPI_Recv(&tbuff, 1000, MPI_INT, src, 7, mcmpi_comm, MPI_STATUS_IGNORE); 
                wait_exit_prologue = 1;
                sem_post(&sem_wait_exit_prologue);
            }
        }else if (tag == 8){ // kill
            int rec;
            MPI_XRecv(&rec, 1, MPI_INT, src, 8, mcmpi_comm, MPI_STATUS_IGNORE); 
            return;
        }
    }     
}

void remove_rank(int lower, int upper){ // local rank
    MPI_Group world_group; 
    MPI_Comm_group(mcmpi_comm, &world_group);
    MPI_Group new_group;
    int oldrank = myrank;
    int ranges[1][3] = { lower, upper, 1 };
    MPI_Group_range_excl(world_group, 1, ranges, &new_group);
    MPI_Comm newworld;
    MPI_Comm_create(mcmpi_comm, new_group, &newworld);
    mcmpi_comm = newworld;
    wrld = mcmpi_comm;
    if (newworld == MPI_COMM_NULL){ 
        MPI_Finalize();
        exit(0);
    }
    int rank;
    MPI_NComm_rank(mcmpi_comm, &rank);
    myrank = rank;
}


void sync_table(){
    int i;
    for (i = 1; i < nodes.size; i++){
        if (strcmp(nodes.comm[i], "global") == 0){
            MPI_NSend(&nodes, 
                    sizeof(nodes), 
                    MPI_CHAR, 
                    nodes.local_rank[i], 
                    21, 
                    mcmpi_comm);
        }
    }
}
