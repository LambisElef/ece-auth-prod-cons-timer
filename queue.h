#ifndef PROD_CONS_TIMER_QUEUE_H
#define PROD_CONS_TIMER_QUEUE_H

#include <stdlib.h>
#include <pthread.h>

typedef struct {
    void *(*work)(void *);
    void *arg;
    struct timeval start;
} WorkFunction;

typedef struct {
    WorkFunction *buf;
    long size;
    long head, tail;
    int full, empty;
    pthread_mutex_t *mut;
    pthread_cond_t *notFull, *notEmpty;
    int *toWrite;
} Queue;

Queue *queueInit(int size, int *toWrite);

void queueDelete(Queue *q);

void queueAdd(Queue *q, WorkFunction in);

void queueDel(Queue *q, WorkFunction *out);

#endif //PROD_CONS_TIMER_QUEUE_H
