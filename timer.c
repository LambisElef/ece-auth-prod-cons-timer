/*
 *	File	: timer.c
 *
 *	Author	: Eleftheriadis Charalampos
 *
 *	Date	: 23 July 2020
 */

#include "timer.h"

void timerInit(Timer *timer, int period, int tasksToExecute, int startDelay, void *(*stopFcn)(void *arg),
        void *(*timerFcn)(), void *(*errorFcn)(), Queue *queue, void *(*producer)(void *arg), int *tJobIn,
        int *tDrift, pthread_mutex_t *tMut) {
    timer->period = period;
    timer->tasksToExecute = tasksToExecute;
    timer->startDelay = startDelay;
    timer->timerFcn = timerFcn;
    timer->errorFcn = errorFcn;

    timer->queue = queue;
    timer->producer = producer;
    timer->tJobIn = tJobIn;
    timer->tDrift = tDrift;
    timer->tMut = tMut;
}



void start(Timer *timer) {
    pthread_create(&timer->tid, NULL, timer->producer, timer);
}

