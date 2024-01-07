
int MPI_XSend(const void *buf, int count, MPI_Datatype type, int dest,
        int tag, MPI_Comm comm){
    //   __  __ ____ ___    __  ______                 _ 
    //  |  \/  |  _ \_ _|   \ \/ / ___|  ___ _ __   __| |
    //  | |\/| | |_) | |     \  /\___ \ / _ \ '_ \ / _` |
    //  | |  | |  __/| |     /  \ ___) |  __/ | | | (_| |
    //  |_|  |_|_|  |___|___/_/\_\____/ \___|_| |_|\__,_|
    //                 |_____|                           

    if (DEBUG) printf(">>[%s][%d][%d]>>MPI_Send> Injecting in MPI_Send mcmpi_app=%d mcmpi_cluster=%d dest=%d tag=%d init=%d %p\n", processor_name, global_rank, mcmpi_cluster_gateway, mcmpi_app, mcmpi_cluster, dest, tag, init, mcmpi_comm); fflush(stdout);
    mpi_send_f_type mpi_send;
    mpi_send = (mpi_send_f_type)dlsym(RTLD_NEXT,"MPI_Send");
    mpi_recv_f_type mpi_recv;
    mpi_recv = (mpi_recv_f_type)dlsym(RTLD_NEXT,"MPI_Recv");


    //if(mcmpi_app && mcmpi_first_message == 0 && !mcmpi_cluster){
    //if((global_rank == 0 && init == 0) || (mcmpi_app && mcmpi_first_message == 0 && init == 0)){
    int tags = 0;
    int dests = 0;
    int rs = 0;
    VB(("Inject MPI_Send? mcmpi_app %d skip_route %d\n", mcmpi_app, skip_route));
    if (mcmpi_app && skip_route == 0){
        VB(("Inject MPI_Send\n"));
        
        // router
        int msg_type_size;
        MPI_Type_size(type, &msg_type_size);
        struct Node * node_from = fetch_node_from_gr(global_rank);
        struct Node * node_to = fetch_node_from_gr(dest);
        VB(("[S] node from: %s %d %d %d %s\n", node_from->hostname, 
                                               node_from->type, 
                                               node_from->local_rank, 
                                               node_from->global_rank, 
                                               node_from->comm));
        VB(("[S] node_to: %s %d %d %d %s\n", node_to->hostname, 
                                             node_to->type, 
                                             node_to->local_rank, 
                                             node_to->global_rank, 
                                             node_to->comm));
        if (strcmp(node_from->comm, node_to->comm) == 0 || 
                (strcmp(node_from->comm, "global") == 0 && node_to->type == 1)){
            VB(("MPI SEND CASE 1 -> %s %d\n", node_to->hostname, node_to->global_rank));
            VB(("[S] from %d to %d\n", node_from->local_rank, node_to->local_rank));
            //mpi_send(buf, count, type, node_to->local_rank, tag, mcmpi_comm);
            MPI_Ssend(buf, count, type, node_to->local_rank, tag, mcmpi_comm);
            tags = tag;
            dests = node_to->local_rank;
        }else{ // different comms
            if (mcmpi_cluster){ // inside cluster
                VB((">>>>>MPI SEND CASE 2.1 -> %s %d\n", node_to->hostname, node_to->global_rank));
                //int hashtag = node_from->global_rank + 500 + tag;
                int hashtag = tag;
                int action[6] = {0,msg_type_size,count,hashtag,global_rank,dest};
                show_action_info(action);
                tags = hashtag;
                dests = 0;
                VB(("[S] from %d to %d\n", node_from->local_rank, 
                                           node_to->local_rank));
                mpi_send(action, 6, MPI_INT, 0, 100, mcmpi_comm);
                mpi_send(buf, count, type, 0, 100, mcmpi_comm);
                int ok; 
                mpi_recv(&ok, 
                         1, 
                         MPI_INT, 
                         0,
                         hashtag, 
                         mcmpi_comm,
                         MPI_STATUS_IGNORE);
                //if (mcmpi_cluster_gateway == 0){
                //    int aaaa = 2;
                //    if (DEBUG) printf("[S/R] from %d to %d\n", node_to->local_rank, node_from->local_rank);
                //    MPI_Recv(&aaaa, 1, MPI_INT, node_to->global_rank, 0, mcmpi_comm, MPI_STATUS_IGNORE);
                //}
                VB(("<<<<<MPI SEND CASE 2.1\n"));
            }else{  // different comms / no cluster
                VB((">>>>>MPI SEND CASE 2.2\n"));
                struct Node * node_gateway = fetch_gateway_comm_node(node_to->comm);
                VB(("[S] gateway_node: %s %d %d %d %s\n", node_gateway->hostname, 
                                                          node_gateway->type, 
                                                          node_gateway->local_rank, 
                                                          node_gateway->global_rank, 
                                                          node_gateway->comm));
                //int hashtag = node_from->global_rank + 500 + tag;
                int hashtag = tag;
                int action[6] = {0,msg_type_size,count,hashtag,global_rank,dest};
                show_action_info(action);
                tags = hashtag;
                dests = node_gateway->local_rank;
                VB(("[S] header from (%s)%d to (%s)%d\n", 
                            node_from->hostname,
                            node_from->local_rank, 
                            node_gateway->hostname,
                            node_gateway->local_rank));
                mpi_send(action, 6, MPI_INT, node_gateway->local_rank, 100, mcmpi_comm);
                VB(("[S] body from (%s)%d to (%s)%d\n", 
                            node_from->hostname,
                            node_from->local_rank, 
                            node_gateway->hostname,
                            node_gateway->local_rank));
                mpi_send(buf, count, type, node_gateway->local_rank, 100, mcmpi_comm);

                VB(("[S] now waiting recv\n"));
                int ok; 
                mpi_recv(&ok, 
                         1, 
                         MPI_INT, 
                         node_gateway->local_rank, 
                         hashtag, 
                         mcmpi_comm,
                         MPI_STATUS_IGNORE);
                //if (mcmpi_cluster_gateway == 0){
                //    int aaaa = 2;
                //    if (DEBUG) printf("[S/R] from %d to %d\n", node_to->local_rank, node_from->local_rank);
                //    MPI_Recv(&aaaa, 1, MPI_INT, dest, 0, mcmpi_comm, MPI_STATUS_IGNORE);
                //}
                VB(("<<<<<MPI SEND CASE 2.2\n"));
            }
        }
    }else{
        VB(("MPI_SEND NATIVE dest=%d tag=%d\n", dest, tag));
        mcmpi_first_message = 1;
        //rs = mpi_send(buf, count, type, dest, tag, comm);
        rs = MPI_Ssend(buf, count, type, dest, tag, comm);
        VB(("MPI_SEND NATIVE >>>>> dest=%d tag=%d\n", dest, tag));
        tags = tag;
        dests = dest;
    }
    if (DEBUG) printf(">>[%s][%d][%d]>>MPI_Send tag[%d] dest[%d]> Injecting OK in MPI_Send mcmpi_app=%d mcmpi_cluster=%d dest=%d tag=%d init=%d %p\n", processor_name, global_rank, mcmpi_cluster_gateway, tags, dests, mcmpi_app, mcmpi_cluster, dest, tag, init, mcmpi_comm); fflush(stdout);
    //}
    return rs;
}

int MPI_XRecv(void *buf, int count, MPI_Datatype type, int source, int tag, MPI_Comm comm, MPI_Status *status){
    //   __  __ ____ ___    __  ______                 
    //  |  \/  |  _ \_ _|   \ \/ /  _ \ ___  _____   __
    //  | |\/| | |_) | |     \  /| |_) / _ \/ __\ \ / /
    //  | |  | |  __/| |     /  \|  _ <  __/ (__ \ V / 
    //  |_|  |_|_|  |___|___/_/\_\_| \_\___|\___| \_/  
    //                 |_____|                         

    mpi_recv_f_type mpi_recv;
    mpi_recv = (mpi_recv_f_type)dlsym(RTLD_NEXT,"MPI_Recv");
    if (DEBUG) printf(">>[%s][%d][%d]>>MPI_Recv> Injecting in MPI_Recv skip_route=%d mcmpi_app=%d mcmpi_cluster=%d src=%d tag=%d init=%d %p\n", processor_name, global_rank, mcmpi_cluster_gateway, skip_route, mcmpi_app, mcmpi_cluster, source, tag, init, mcmpi_comm); fflush(stdout);
    //if(mcmpi_app && mcmpi_first_message == 0 && init == 0){
    int rs = 0;
    VB(("Inject MPI_Recv? mcmpi_app %d skip_route %d\n", mcmpi_app, skip_route));
    if(mcmpi_app && skip_route == 0){
        struct Node * node_src = fetch_node_from_gr(source);
        VB(("[R] node source: %s %d %d %d %s\n", node_src->hostname, 
                                                 node_src->type, 
                                                 node_src->local_rank, 
                                                 node_src->global_rank, 
                                                 node_src->comm));
        struct Node * my_node = fetch_node_from_gr(global_rank);
        VB(("[R] my_node: %s %d %d %d %s\n", my_node->hostname, 
                                             my_node->type, 
                                             my_node->local_rank, 
                                             my_node->global_rank, 
                                             my_node->comm));
        if (strcmp(my_node->comm, node_src->comm) == 0){
            VB(("RECV CASE 1 - Same comm\n")); 
            VB(("[%s][RECV] src=%d tag=%d\n", processor_name, 
                                              node_src->local_rank, 
                                              tag));
            rs = mpi_recv(buf, 
                          count, 
                          type, 
                          node_src->local_rank, 
                          tag, 
                          mcmpi_comm, 
                          status);
        }else if (strcmp(my_node->comm, "global") == 0){ // "global" <- "cluster"
            VB(("RECV CASE 2 - Global comm\n"));
            struct Node * node_gateway = fetch_gateway_comm_node(node_src->comm);
            //int hashtag = node_src->global_rank + 500 + tag;
            int hashtag = tag;
            VB(("[%s][RECV] src=%d tag=%d\n", processor_name, 
                                              node_gateway->local_rank, 
                                              hashtag));
            rs = mpi_recv(buf, 
                           count, 
                           type, 
                           node_gateway->local_rank, 
                           hashtag, 
                           mcmpi_comm,
                           status);
            VB(("[%s][RECV] src=%d tag=%d OK Sending ACK back %d\n", processor_name, 
                                                 node_gateway->local_rank, 
                                                 hashtag,
                                                 source));
            // ACK Recv
            //int action[6] = {101,source,0,0,0,0};
            //show_action_info(action);
            //MPI_Request treq;
            //MPI_Isend(action, 
            //          6, 
            //          MPI_INT, 
            //          node_gateway->local_rank, 
            //          100,
            //          mcmpi_comm, 
            //          &treq);
            //
            //
            //if (mcmpi_cluster_gateway == 0){
            //    if (DEBUG) warn("RECV ACK CASE 2");
            //    int aaaa = 2;
            //    MPI_Send(&aaaa, 1, MPI_INT, node_src->global_rank, 0, mcmpi_comm);
            //}
        }else{ // "global" -> "cluster"
            VB(("RECV CASE 3 global -> cluster from %d\n", source));
            //int hashtag = node_src->global_rank + 500 + tag;
            int hashtag = tag;
            VB(("[%s][RECV] src=%d tag=%d\n", 
                        processor_name, 0, hashtag));
            rs = mpi_recv(buf, count, type, 0, hashtag, mcmpi_comm, status);
            VB(("[%s][RECV] src=%d tag=%d OK. Sending ACK back %d.\n", 
                        processor_name, 0, hashtag,source));
            // ACK Recv
            //int action[6] = {101,source,0,0,0,0};
            //show_action_info(action);
            //MPI_Request treq;
            //MPI_Isend(action, 6, MPI_INT, 0, 100, mcmpi_comm, &treq);
            //
            //
            //if (mcmpi_cluster_gateway == 0){
            //    if (DEBUG) warn("RECV ACK CASE 3");
            //    int aaaa = 2;
            //    MPI_Send(&aaaa, 1, MPI_INT, node_src->global_rank, 0, mcmpi_comm);
            //}
        }
    }else{
        VB(("RECV NATIVE src=%d tag=%d\n", source, tag));
        mcmpi_first_message = 1;
        if (DEBUG) printf("[RECV] src=%d tag=%d\n", source, tag); fflush(stdout);
        rs = mpi_recv(buf, count, type, source, tag, comm, status);
        VB(("RECV NATIVE <<<< src=%d tag=%d\n", source, tag));
    }
    return rs;
}



int MPI_XBarrier(MPI_Comm comm){

    //   __  __ ____ ___    __  ______                  _           
    //  |  \/  |  _ \_ _|   \ \/ / __ )  __ _ _ __ _ __(_) ___ _ __ 
    //  | |\/| | |_) | |     \  /|  _ \ / _` | '__| '__| |/ _ \ '__|
    //  | |  | |  __/| |     /  \| |_) | (_| | |  | |  | |  __/ |   
    //  |_|  |_|_|  |___|___/_/\_\____/ \__,_|_|  |_|  |_|\___|_|   
    //                 |_____|                                      
    
    mpi_barrier_f_type mpi_barrier;
    mpi_barrier = (mpi_comm_rank_f_type)dlsym(RTLD_NEXT,"MPI_Barrier");
    int rs = 0;
    if (mcmpi_app == 0)
        mpi_barrier(comm);

    VB(("INJECTING MPI_BARRIER mcmpi_cluster(%d)\n", mcmpi_cluster));
    if (mcmpi_cluster == 0){
        int i;
        for (i = 0; i < nodes.size; i++){
            if (nodes.type[i] != 1) continue;
            int * tbuff = (int*) malloc(sizeof(int)*1000);
            MPI_Request treq;
            VB(("Sending ACT15 to %s %d\n",
                            nodes.hostname[i],
                            nodes.local_rank[i]));
            int action[6] = {15,0,0,0,0,0};
            show_action_info(action);
            MPI_Isend(action, 6, MPI_INT, nodes.local_rank[i], 100, mcmpi_comm, &treq);
            //MPI_Isend(tbuff, 1000, MPI_INT, nodes.local_rank[i], 15, mcmpi_comm, &treq);
        }
        mpi_barrier(mcmpi_comm); 
    }else{ 
        // Only ClusterNodes will execute here
        // Gateway1 will run in exec_command
        // Gateway2 will run in thread_work
        // send warning to gateway2
        // when it becomes full; which is #msgs = size-1 (excluding gateway)
        // then it unlocks MPI_Barrier
        if (mcmpi_cluster_gateway == 1 || mcmpi_cluster_gateway == 2){
            mpi_barrier(mcmpi_comm);
        }else{
            int * tbuff = (int*) malloc(sizeof(int)*1000);
            MPI_Request treq;
            VB(("Sending ACT15 to gateway2\n"));
            int action[6] = {15,0,0,0,0,0};
            show_action_info(action);
            MPI_Isend(action, 6, MPI_INT, 0, 100, mcmpi_comm, &treq);
            //MPI_Isend(tbuff, 1000, MPI_INT, 0, 15, mcmpi_comm, &treq);
            VB(("Waiting MPI_Barrier001\n"));
            mpi_barrier(mcmpi_comm);
            VB(("Leaving MPI_Barrier001\n"));
        }
    }
    VB(("LEAVING MPI_BARRIER\n"));

    return rs;
    //VB(("INJECTING MPI_BARRIER"));
    // Receive from others
    //int i,j;
    //int * tbuff = (int*) malloc(sizeof(int)*1000);
    //if (global_rank == 0){
    //    for (i = 0; i < nodes.size; i++){
    //        if (nodes.global_rank[i] == 0 || nodes.type[i] == 1) continue;
    //        MPI_Recv(tbuff, 1000, MPI_INT, nodes.global_rank[i], 0, mcmpi_comm, MPI_STATUS_IGNORE);
    //    }
    //    //sleep(60);
    //    for (i = 0; i < nodes.size; i++){
    //        if (nodes.global_rank[i] == 0 || nodes.type[i] == 1) continue;
    //        MPI_Send(tbuff, 1000, MPI_INT, nodes.global_rank[i], 0, mcmpi_comm);
    //    }
    //}else{
    //    MPI_Send(tbuff, 1000, MPI_INT, 0, 0, mcmpi_comm);
    //    MPI_Recv(tbuff, 1000, MPI_INT, 0, 0, mcmpi_comm, MPI_STATUS_IGNORE);
    //}
    //VB(("LEAVING MPI_BARRIER"));
    return rs;
}
