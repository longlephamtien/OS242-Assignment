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
#include <string.h>

int __sys_killall(struct pcb_t *caller, struct sc_regs *regs)
{
    char proc_name[100];
    uint32_t data;

    // hardcode for demo only
    uint32_t memrg = regs->a1;

    /* TODO: Get name of the target proc */
    // proc_name = libread..
    int i = 0;
    data = 0;
    while (data != -1)
    {
        libread(caller, memrg, i, &data);
        proc_name[i] = data;
        if (data == -1)
            proc_name[i] = '\0';
        i++;
    }
    printf("The procname retrieved from memregionid %d is \"%s\"\n", memrg, proc_name);

    /* TODO: Traverse proclist to terminate the proc
     *       stcmp to check the process match proc_name
     */
    // caller->running_list
    // caller->mlq_ready_queu

    if (caller->running_list == NULL || caller->running_list->size == 0)
        return -1;

    struct queue_t *q = caller->running_list;
    if (!empty(q))
    {
        for (int i = 0; i < q->size; i++)
        {
            char proc_name_in_queue[100];
            int j = 0;
            uint32_t temp = 0;

            while (temp != -1)
            {
                libread(caller, q->proc[i]->regs[0], j, &temp);
                if (temp == -1)
                {
                    proc_name_in_queue[j] = '\0';
                    break;
                }
                proc_name_in_queue[j] = temp;
                j++;
            }

            /* TODO Maching and terminating
             *       all processes with given
             *        name in var proc_name
             */

            if (strcmp(proc_name, proc_name_in_queue) == 0)
            {
                struct pcb_t *proc = q->proc[i];
                for (int t = i; t < q->size - 1; t++)
                {
                    q->proc[t] = q->proc[t + 1];
                }
                libfree(proc, proc->regs[0]);
            }
        }
    }

    return -1;
}