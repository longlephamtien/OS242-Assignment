#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
        if (q == NULL) return 1;
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
        /* TODO: put a new process to queue [q] */
        spin_lock_lock(&q->lock); // Acquire the lock for thread safety
        if (q == NULL || proc == NULL) return;
    
        // Check Queue full?
        if (q->size >= MAX_QUEUE_SIZE) {
                fprintf(stderr, "Queue is full. Cannot enqueue process %d\n", proc->pid);
                return;
        }

        // Add the new process to the end of the queue
        q->proc[q->size] = proc;
        q->size++;

        // Rearrange the queue to maintain the priority order
        for (int i = q->size - 1; i > 0; i--) {
                if (q->proc[i]->priority < q->proc[i - 1]->priority) {
                        // Hoán đổi vị trí
                        struct pcb_t *temp = q->proc[i];
                        q->proc[i] = q->proc[i - 1];
                        q->proc[i - 1] = temp;
                } else {
                        break;
                }
        }
        spin_lock_unlock(&q->lock); // Release the lock
}

struct pcb_t * dequeue(struct queue_t * q) {
        /* TODO: return a pcb whose prioprity is the highest
         * in the queue [q] and remember to remove it from q
         * */
        spin_lock_lock(&q->lock); // Acquire the lock for thread safety
	if (empty(q)) return NULL;
        struct pcb_t * proc = q->proc[0];
        // Shift all processes to the front of the queue
        for (int i = 0; i < q->size - 1; i++) {
                q->proc[i] = q->proc[i + 1];
        }
        
        q->size--; // Decrease the size of the queue
        q->proc[q->size] = NULL; // Clear the last position
        spin_lock_unlock(&q->lock); // Release the lock
        return proc;
}

