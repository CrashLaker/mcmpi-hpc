

void remove_rank_sem_init(){
    sem_init(&sem_remove,0,1);  // valor 1
    sem_init(&sem_communication,0,1);  // valor 1
    sem_init(&excl_count_communication,0,1);  // valor 1
}

void remove_rank_sem_finalize(){
    sem_destroy(&sem_remove);
    sem_destroy(&sem_communication);
}

void thread_work_remove_rank_sem_prologue(){
    sem_wait(&sem_remove);
    sem_wait(&sem_communication);
    sem_post(&sem_communication);
}

void thread_work_remove_rank_sem_epilogue(){
    sem_post(&sem_remove);
}

void mpi_remove_rank_sem_prologue(){
    //VB(("enter mpi_remove_rank_sem_prologue\n"));
    //VB(("mpi_remove_rank_sem_prologue 1\n"));
    sem_wait(&sem_remove);
    //VB(("mpi_remove_rank_sem_prologue 2\n"));
    sem_wait(&excl_count_communication);
    //VB(("mpi_remove_rank_sem_prologue 3\n"));
    if (count_communication==0)
        sem_wait(&sem_communication);
    sem_post(&sem_remove);
    count_communication++;
    sem_post(&excl_count_communication);
}

void mpi_remove_rank_sem_epilogue(){
    //VB(("enter mpi_remove_rank_sem_epilogue\n"));
    sem_wait(&excl_count_communication);
    count_communication--;
    if (count_communication==0)
        sem_post(&sem_communication);
    sem_post(&excl_count_communication);
}

void mcmpi_scale_on(){
    MPI_Barrier(mcmpi_comm);
    if (mcmpi_scale_on_val == 1){
        VB(("thread already up!\n"));
        return;
    }

    if (global_rank == 0){
        int i;
        for (i = 0; i < nodes.size; i++){
            if (nodes.type[i] != 1) continue; // type 1 = gateways
            VB(("Sending ACT30 to %s %d\n",
                            nodes.hostname[i],
                            nodes.local_rank[i]));
            MPI_Request treq;
            int action[6] = {30,0,0,0,0,0};
            MPI_Isend(action,  
                     6, 
                     MPI_INT, 
                     nodes.local_rank[i], 100, mcmpi_comm, &treq);
        }
    }else{
        mcmpi_scale_on_val = 1;
        VB(("Starting new thread\n"));
        if (pthread_create(&tid_thread_work, NULL, thread_work, NULL)){
            VB(("Cannot create thread\n"));
        }
    }
}


void mcmpi_scale_off(){
    MPI_Barrier(mcmpi_comm);
    if (mcmpi_scale_on_val == 0){
        VB(("thread already down!\n"));
        return;
    }
    if (global_rank == 0){
        int i;
        for (i = 0; i < nodes.size; i++){
            if (nodes.type[i] != 1) continue; // type 1 = gateways
            VB(("Sending ACT31 to %s %d\n",
                            nodes.hostname[i],
                            nodes.local_rank[i]));
            MPI_Request treq;
            int action[6] = {31,0,0,0,0,0};
            MPI_Isend(action,  
                     6, 
                     MPI_INT, 
                     nodes.local_rank[i], 100, mcmpi_comm, &treq);
        }
    }else{
        mcmpi_scale_on_val = 0;
    }
}

void mcmpi_remove_nranks(int size){ 
    _mcmpi_remove_rank(nodes.size-size, nodes.size-1); // lower, upper
}

void mcmpi_remove_rank(int whichrank){ // whichrank is global_rank
    _mcmpi_remove_rank(whichrank, whichrank); 
}

void mcmpi_remove_cluster(char * clustername){
    VB(("to remove cluster %s\n", clustername));
    int i;
    int removerank;
    for (i = 1; i < nodes.size; i++){
        if (nodes.type[i] == 1 && strcmp(nodes.hostname[i], clustername) == 0){
            removerank = nodes.local_rank[i]; 
            break;
        }
    }
    _mcmpi_remove_rank(removerank, removerank);
}

void _mcmpi_remove_rank(int lower, int upper){ // whichrank is global_rank
    if (global_rank == 0){
        VB(("_mcmpi_remove_rank CASE 1 global_rank\n"));
        int i, y; 
        MPI_Request treq;
        int payload[2] = {lower, upper}; //lower bound, upper bound
        for (i = 1; i < nodes.size; i++){
            if (nodes.type[i] != 0) continue;
            MPI_XSend(payload, 2, MPI_INT, nodes.global_rank[i], 22, mcmpi_comm);
        }
        MPI_Status status;
        int flag_controle = 1;
        int dummy = 1;
        int *controle_com = (int*) calloc(nodes.size, sizeof(int));
        VB(("start while flag_controle\n"));
        while(flag_controle!=0){
            flag_controle = 0;
            int ret = 0;
            for (i = 1; i < nodes.size; i++){
                if (nodes.type[i] != 0) continue;
                VB(("to recv tag200 from %d\n", nodes.global_rank[i]));
                MPI_XRecv(&ret, 1, MPI_INT, nodes.global_rank[i], 200, mcmpi_comm, &status);
                VB(("ok recv tag200 from %d val %d\n", nodes.global_rank[i], ret));
                controle_com[i] = ret;
                if (ret == 1)
                    flag_controle = 1;

            }
            if (flag_controle == 0){ // OK pode seguir para o exclude
                for (i = 1; i < nodes.size; i++){
                    if (strcmp(nodes.comm[i], "global") == 0) continue;
                    VB(("OK>>>> to send ok to %d\n", nodes.global_rank[i]));
                    if (nodes.type[i] == 0){
                            MPI_XSend(&flag_controle, 1, MPI_INT, nodes.global_rank[i], 201, mcmpi_comm);
                    }else{ // cluster gateway
                        int msg[6] = {22,lower,upper,0,0,0};
                        MPI_NSend(msg, 6, MPI_INT, nodes.local_rank[i], 100, mcmpi_comm);

                    }
                    VB(("OK OK>>>> to send ok to %d\n", nodes.global_rank[i]));
                }
                for (i = 1; i < nodes.size; i++){
                    if (strcmp(nodes.comm[i], "global") != 0) continue;
                    VB(("OK>>>> to send ok to %d\n", nodes.global_rank[i]));
                    if (nodes.type[i] == 0){
                            MPI_XSend(&flag_controle, 1, MPI_INT, nodes.global_rank[i], 201, mcmpi_comm);
                    }else{ // cluster gateway
                        int msg[6] = {22,lower,upper,0,0,0};
                        MPI_NSend(msg, 6, MPI_INT, nodes.local_rank[i], 100, mcmpi_comm);

                    }
                    VB(("OK OK>>>> to send ok to %d\n", nodes.global_rank[i]));
                }
            }else{
                for (i = 1; i < nodes.size; i++){
                    if (nodes.type[i] != 0) continue;
                    if (controle_com[i] == 1){ // ainda em comunicacao
                        flag_controle = 2;
                        VB(("to send tag201 to %d val %d\n", nodes.global_rank[i], flag_controle));
                        MPI_XSend(&flag_controle, 1, MPI_INT, nodes.global_rank[i], 201, mcmpi_comm);
                        VB(("ok to send tag201 to %d\n", nodes.global_rank[i]));
                    }else if (controle_com[i] == 0){ // node ok
                        flag_controle = 1;
                        VB(("to send tag201 to %d val %d\n", nodes.global_rank[i], flag_controle));
                        MPI_XSend(&flag_controle, 1, MPI_INT, nodes.global_rank[i], 201, mcmpi_comm);
                        VB(("ok to send tag201 to %d\n", nodes.global_rank[i]));
                    }
                }
                for (i = 1; i < nodes.size; i++){
                    if (nodes.type[i] != 0) continue;
                    if (controle_com[i] == 1){ // ainda em comunicacao
                        VB(("to receive tag202 from %d v 1\n", nodes.global_rank[i]));
                        MPI_XRecv(&dummy, 1, MPI_INT, nodes.global_rank[i], 202, mcmpi_comm, &status);
                        VB(("ok to receive tag202 from %d v1\n", nodes.global_rank[i]));
                    }
                }
                for (i = 1; i < nodes.size; i++){
                    if (nodes.type[i] != 0) continue;
                    if (controle_com[i] == 0){ // ainda em comunicacao
                        VB(("to receive tag202 from %d v 2\n", nodes.global_rank[i]));
                        MPI_XSend(&dummy, 1, MPI_INT, nodes.global_rank[i], 202, mcmpi_comm);
                        VB(("ok to receive tag202 from %d v 2\n", nodes.global_rank[i]));
                    }
                }
            }
        }
        thread_work_remove_rank_sem_prologue();
        remove_rank(lower, upper);
        thread_work_remove_rank_sem_epilogue();
        VB(("L0\n"));
        int whichrank = lower;
        if (nodes.type[whichrank] == 1){ // remove cluster
            VB(("L1\n"));
            int clusternodecount = 0;
            int active = 0;
            i = whichrank;
            char clustername[100];
            strcpy(clustername, nodes.hostname[i]);
            dump_nodes(&nodes);
            VB(("to remove cluster nodes + gateway\n"));
            while(strcmp(nodes.hostname[i], clustername) == 0 || 
                  strcmp(nodes.comm[i], clustername) == 0){
                VB(("here>>1 i=%d lr=%d gr=%d hostname=%s comm=%s\n", i, nodes.local_rank[i], nodes.global_rank[i], nodes.hostname[i], nodes.comm[i]));
                if (i == nodes.size-1){
                    // if last just decrease nodes.size
                    VB(("i == nodes.size-1\n"));
                    nodes.size--;
                    nodes.grank_size--;
                    break;
                }else{
                    VB(("here>>2 i=%d lr=%d gr=%d hostname=%s comm=%s\n", i+1, nodes.local_rank[i+1], nodes.global_rank[i+1], nodes.hostname[i+1], nodes.comm[i+1]));

                    int c_is_cluster = nodes.type[i];
                    for (y = i; y < nodes.size-1; y++){
                        strcpy(nodes.hostname[y], nodes.hostname[y+1]);
                        nodes.type[y] = nodes.type[y+1];
                        
                        if (c_is_cluster == 1){
                            if (strcmp(nodes.comm[y+1], "global") == 0)
                                nodes.local_rank[y] = nodes.local_rank[y+1]-1;
                            else
                                nodes.local_rank[y] = nodes.local_rank[y+1];
                        }else{
                            nodes.local_rank[y] = nodes.local_rank[y+1];
                        }

                        if (c_is_cluster == 0){
                            if (nodes.type[y+1] == 1)
                                nodes.global_rank[y] = nodes.global_rank[y+1];
                            else
                                nodes.global_rank[y] = nodes.global_rank[y+1]-1;
                        }else{
                            nodes.global_rank[y] = nodes.global_rank[y+1];
                        }
                        strcpy(nodes.comm[y], nodes.comm[y+1]);
                    }
                }
                dump_nodes(&nodes);
                nodes.size--;
                nodes.grank_size--;
            }
            dump_nodes(&nodes);
        }else{
            for (whichrank = upper; whichrank >= lower; whichrank--){
                for (i = whichrank+1; i < nodes.size; i++){
                    strcpy(nodes.hostname[i-1], nodes.hostname[i]);
                    nodes.type[i-1] = nodes.type[i];
                    if (strcmp(nodes.comm[i], "global") == 0)
                        nodes.local_rank[i-1] = nodes.local_rank[i]-1;
                    else
                        nodes.local_rank[i-1] = nodes.local_rank[i];

                    if (nodes.type[i] == 1)
                        nodes.global_rank[i-1] = nodes.global_rank[i];
                    else
                        nodes.global_rank[i-1] = nodes.global_rank[i]-1;
                    strcpy(nodes.comm[i-1], nodes.comm[i]);
                }
                nodes.size--;
                nodes.grank_size--;
            }
        }
        sync_table();
    }else if(mcmpi_cluster_gateway == 1){ // global
        VB(("_mcmpi_remove_rank CASE 2 cluster gateway global\n"));
        remove_rank(lower, upper);
    }else if(mcmpi_cluster_gateway == 2){ // cluster
        VB(("_mcmpi_remove_rank CASE 3 cluster gateway cluster\n"));
        VB(("TODO remove_rank cluster gateway\n"));
    }else{ // other globals
        VB(("_mcmpi_remove_rank CASE 4 other user\n"));
        VB(("received remove rank %d:%d\n", lower, upper));
        //thread_work_remove_rank_sem_prologue();
        int flag_wait_remove = 1;
        int flag_count;
        int dummy = 1;
        MPI_Status status;
        VB(("before start while flag_wait_remove\n"));
        while(flag_wait_remove != 0){
            if (flag_wait_remove != 1) sem_wait(&sem_remove);
            sem_wait(&excl_count_communication);
            if (count_communication == 0)
                flag_count = 0;
            else
                flag_count = 1;
            sem_post(&excl_count_communication);
            VB(("to send to 0 val %d\n", flag_count));
            MPI_XSend(&flag_count, 1, MPI_INT, 0, 200, mcmpi_comm);
            VB(("OK to send to 0\n"));
            VB(("to recv from 0\n"));
            MPI_XRecv(&flag_wait_remove, 1, MPI_INT, 0, 201, mcmpi_comm, &status);
            VB(("OK to recv from 0 val %d\n", flag_wait_remove));
            if (flag_wait_remove == 1){ // ele esta OK mas outros nao
                sem_post(&sem_remove);
                MPI_XRecv(&dummy, 1, MPI_INT, 0, 202, mcmpi_comm, &status);
            }else if(flag_wait_remove == 2){ // count_comm <> 0
                sem_wait(&sem_communication);
                sem_post(&sem_communication);
                MPI_XSend(&dummy, 1, MPI_INT, 0, 202, mcmpi_comm);
                flag_count = 0;
            }
        }
        if (!mcmpi_cluster){
            remove_rank(lower, upper);
        }else{ // cluster node
            if (nodes.type[lower] == 1 && 
                    strcmp(nodes.hostname[lower], nodes.comm[gr2lr(myrank)]) == 0){ // cluster shutdown
                VB(("cluster shutdown\n"));
                VB(("to die\n"));
                MPI_Finalize();
                exit(0);
            }
        }
        sem_post(&sem_remove);
        //thread_work_remove_rank_sem_epilogue();
        if (global_rank > lower){
            global_rank--;
        }
    }
}





















