/*
 *	File	: timer.c
 *
 *	Author	: Eleftheriadis Charalampos
 *
 *	Date	: 23 July 2020
 */

#include "timer.h"

void startFcn(Timer *timer, int period, int tasksToExecute, int startDelay, void *(*timerFcn)(void *arg), Queue *queue, void *(*producer)(void *arg), int *tJobIn, int *tDrift) {
    timer->period = period;
    timer->tasksToExecute = tasksToExecute;
    timer->startDelay = startDelay;
    timer->timerFcn = timerFcn;

    timer->queue = queue;
    timer->producer = producer;
    timer->tJobIn = tJobIn;
    timer->tDrift = tDrift;
}

void stopFcn(Timer *timer) {
    free(timer);
}

void start(Timer *timer) {
    pthread_t pro;
    pthread_create(&pro, NULL, timer->producer, timer);
    pthread_detach(pro);
}

