#ifndef PROD_CONS_TIMER_TIMER_H
#define PROD_CONS_TIMER_TIMER_H

#include "queue.h"

typedef struct {
    int period;
    int tasksToExecute;
    int startDelay;
    void *(*timerFcn)(void *arg);

    Queue *queue;
    void *(*producer)(void *arg);
} Timer;

void startFcn(Timer *timer, int period, int tasksToExecute, int startDelay, void *(*timerFcn)(void *arg), Queue *queue, void *(*producer)(void *arg));
void stopFcn(Timer *timer);

void start(Timer timer);

void startat(Timer timer, int year, int month, int day, int hour, int minute, int second);

#endif //PROD_CONS_TIMER_TIMER_H
