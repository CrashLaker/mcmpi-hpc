


void task_loop_worker_thread(){
    VB(("Start task_loop_worker_thread\n"));
    int todeletearr[1000];
    int i;
    for (i = 0; i < 1000; i++)
        todeletearr[i] = -1;
    struct NodeTask * current = NULL;
    struct NodeTask * prev = NULL;
    struct NodeTask * aux = NULL;

    while (1) {
        current = head_task;
        prev = NULL;
        aux = NULL;
        int flag;
        int delidx = 0;
        MPI_Status status;
        //VB(("node task start head %p\n", head_task));
        while (current != NULL){
            flag = 0;
            VB(("MPI id(%d) TEST %d req %p\n", current->id, flag, current->req));
            MPI_Test(current->req, &flag, &status);       
            //MPI_Wait(current->req, &status);
            VB(("MPI TEST after %d %p\n", flag, current->req));
            if (flag){
                VB(("Free no malloc do req\n"));
                free(current->req);
                send_command_arg(8,
                                 current->id_buffer,
                                 current->src,
                                 current->tag,0,0,0);
                todeletearr[delidx] = current->id;
                delidx++;
            }
            prev = current;
            current = current->next;
        }
        for (i = 0; i < delidx; i++){
            if (todeletearr[i] == -1) break;
            VB(("to delete task %d\n", todeletearr[i]));
            task_del(todeletearr[i]); 
            todeletearr[i] = -1;
            task_print_list();
        }
        sleep(5);
    }
}

void task_del(int id){
    sem_wait(&task_loop_lock);
    struct NodeTask * current;
    struct NodeTask * prev;
    prev = NULL;
    current = head_task;
    while (current){
        if (current->id == id){ // delete
            if (prev == NULL){
                if (tail_task == current)
                    tail_task = current->next;
                head_task = current->next;
            }else{
                if (current == tail_task){
                    tail_task = prev;
                }
                prev->next = current->next;
            }
            free(current);
            nodetasksize--;
            break;
        } // end if
        current = current->next;
    }
    sem_post(&task_loop_lock);
}

void task_push(int id_buffer, MPI_Request * req, int src, int dest, int tag){
    VB((">>>>>> Inserindo lista src %d dest %d buffer id %d req %p\n", 
                src,
                dest,
                id_buffer, 
                req));
    // action{action, type, size, tag, dest, buffer_id, src}
    sem_wait(&task_loop_lock);
    if (head_task == NULL){
        head_task = malloc(sizeof(struct NodeTask));
        head_task->id = nodetasksize;
        head_task->id_buffer = id_buffer;
        head_task->src = src;
        head_task->dest = dest;
        head_task->req = req;
        head_task->tag = tag;
        tail_task = head_task;
        VB(("Task added id=%d src=%d dest=%d tag=%d %p\n",
                        head_task->id,
                        head_task->src,
                        head_task->dest,
                        head_task->tag,
                        head_task->req));
        head_task->next = NULL;
    }else{
        //struct NodeTask * current = head_task; 
        //while (current->next != NULL){
        //    current = current->next;
        //}

        tail_task->next = malloc(sizeof(struct NodeTask));
        tail_task->next->id = nodetasksize;
        tail_task->next->id_buffer = id_buffer;
        tail_task->next->src = src;
        tail_task->next->dest = dest;
        tail_task->next->tag = tag;
        tail_task->next->req = req;
        tail_task->next->next = NULL;

        tail_task = tail_task->next;
    } 
    nodetasksize++;
    sem_post(&task_loop_lock);
}

void task_print_list(){
    struct NodeTask * current = head_task; 
    while (current != NULL){
        //printf("buffer %p buffer_id %d type %d source %d tag %d\n", current->buffer, current->buffer_id, current->type, current->source, current->tag);
        //printf("buffer %p buffer_id %d"); 
        VB(("%d,%d,src=%d,dest=%d,%p\n", current->id,
                                         current->id_buffer,
                                         current->src,
                                         current->dest,
                                         current->req));
        current = current->next;
    }
}

#if 0
void task_loop_clean(){
    struct NodeTask * current = head_task;
    struct NodeTask * prev = NULL;
    struct NodeTask * aux = NULL;
    int flag;
    MPI_Status status;
    printf("entrou loop\n");
    fflush(0);
    while (current != NULL){
        flag = 0;
        printf("MPI TEST %d req %p\n", flag, current->req);
        fflush(0);
        MPI_Test(current->req, &flag, &status);       
        //MPI_Wait(current->req, &status);
        printf("MPI TEST  after %d %p\n", flag, current->req);
        fflush(0);
        if (flag){
            printf("Free no malloc do req\n");
            fflush(0);
            free(current->req);

        //if (current->req == MPI_REQUEST_NULL){
            //printf("Entra mpi wait\n");
            //fflush(0);
            //MPI_Wait(current->req, &status);
            //printf("sai mpi wait\n");
            //fflush(0);
            printf(">>>>>> DELETA buffer id %d %p\n", current->buffer_id, current->buffer);
            fflush(0);
            printf("shmdt(%p)\n", current->buffer);
            fflush(0);
            shmdt(current->buffer); 
            printf("shmctl(%d)\n", current->buffer_id);
            fflush(0);
            shmctl(current->buffer_id, IPC_RMID, NULL);
            printf("DELETA ok\n");
            fflush(0);
            //printf("free buffer %p before detaching\n", current->buffer);
            //fflush(0);
            //free(current->buffer);
            if (prev == NULL){
                //printf("deleta ok if\n");
                //fflush(0);
                head_task = current->next;
                printf("free current\n");
                fflush(0);
                free(current);
                printf("free current %p ok\n", current);
                fflush(0);
                current = head_task;
                printf("current %p head_task %p\n", current, head_task);
                fflush(0);
            }else{
                //printf("deleta ok else\n");
                //fflush(0);
                aux = current->next;
                free(current);
                current = aux;
            }
        }else{
            prev = current;
            current = current->next;
        }
    }
    printf("saiu loop\n");
    fflush(0);
}
#endif

