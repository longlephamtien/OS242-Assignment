/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

#include "common.h"
#include "syscall.h"
#include "stdio.h"
#include "libmem.h"
#include "mm.h"
#include "string.h"
#include "queue.h"

void match_terminate(struct pcb_t *caller, struct queue_t *queue, const char *proc_name) {
    for (int i = 0; i < queue->size; ) {
        struct pcb_t *pcb = queue->proc[i];
        if (pcb && strcmp(pcb->path, proc_name) == 0) {
            for (int j = i; j < queue->size - 1; j++)
                queue->proc[j] = queue->proc[j + 1];
            queue->size--;
            __free(caller, i, *(int*)(queue->proc[i]->code->text));
            __free(caller, i, *(int*)queue->proc[i]->code);
        } else {
            i++;
        }
    }
}

int __sys_killall(struct pcb_t *caller, struct sc_regs* regs)
{
    char proc_name[100];
    uint32_t data;

    //hardcode for demo only
    uint32_t memrg = regs->a1;
    
    /* TODO: Get name of the target proc */
    //proc_name = libread..
    int i = 0;
    data = 0;
    while(data != -1){
        libread(caller, memrg, i, &data);
        proc_name[i]= data;
        if(data == -1) {
            proc_name[i]='\0';
            break;
        }
        i++;
    }
    printf("The procname retrieved from memregionid %d is \"%s\"\n", memrg, proc_name);

    /* TODO: Traverse proclist to terminate the proc
     *       stcmp to check the process match proc_name
     */
    //caller->running_list
    //caller->mlq_ready_queu

    /* TODO Maching and terminating 
     *       all processes with given
     *        name in var proc_name
     */

    match_terminate(caller, caller->running_list, proc_name);
    match_terminate(caller, caller->mlq_ready_queue, proc_name); 
    match_terminate(caller, caller->ready_queue, proc_name); 
    
    printf("All processes with name \"%s\" have been terminated.\n", proc_name);

    return 0; 
}
