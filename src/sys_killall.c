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
#include "queue.h"

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
    if (caller->ready_queue) {
        for (int i = 0; i < caller->ready_queue->size; ) {
            struct pcb_t *pcb = caller->ready_queue->proc[i];
            if (pcb && strcmp(pcb->path, proc_name) == 0) {
                for (int j = i; j < caller->ready_queue->size - 1; j++)
                    caller->ready_queue->proc[j] = caller->ready_queue->proc[j + 1];
                caller->ready_queue->size--;
                free(pcb);
            } else {
                i++;
            }
        }
    }

    if (caller->running_list) {
        for (int i = 0; i < caller->running_list->size; ) {
            struct pcb_t *pcb = caller->running_list->proc[i];
            if (pcb && strcmp(pcb->path, proc_name) == 0) {
                for (int j = i; j < caller->running_list->size - 1; j++)
                    caller->running_list->proc[j] = caller->running_list->proc[j + 1];
                caller->running_list->size--;
                free(pcb);
            } else {
                i++;
            }
        }
    }
    /* TODO Maching and terminating 
     *       all processes with given
     *        name in var proc_name
     */

    return 0; 
}
