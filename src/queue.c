#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
        if (q == NULL) return 1;
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
        /* TODO: put a new process to queue [q] */
        if (q == NULL || proc == NULL) return;
        if ( q->size == 0){
                q->proc[0] = proc;
        }
        else {
                int i = q->size -1;
                while(i >= 0 && q->proc[i]->priority < proc->priority){
                        q->proc[i+1] = q->proc[i]; //if the priority of the new process is higher than the current one, move the current one to the right, plus 1 because add new proc mean that the size increase 1
                        i--; 
                }
                q->proc[i+1] = proc;
        }

}

struct pcb_t * dequeue(struct queue_t * q) {
        /* TODO: return a pcb whose prioprity is the highest
         * in the queue [q] and remember to remove it from q
         * */

         if (q == NULL || q->size == 0) return NULL;
    
         struct pcb_t * proc = q->proc[0];
         
         for (int i = 0; i < q->size - 1; i++) {
             q->proc[i] = q->proc[i + 1];
         }
         
         q->size--;
         
         return proc;
}
