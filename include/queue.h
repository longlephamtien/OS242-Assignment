
#ifndef QUEUE_H
#define QUEUE_H

#include "common.h"

#include <pthread.h> // Include pthread for spinlock_t

#define MAX_QUEUE_SIZE 10

struct queue_t {
	struct pcb_t * proc[MAX_QUEUE_SIZE];
	int size;
	
	spinlock_t lock; // Spinlock for thread safety
};

void enqueue(struct queue_t * q, struct pcb_t * proc);

struct pcb_t * dequeue(struct queue_t * q);

int empty(struct queue_t * q);

#endif

