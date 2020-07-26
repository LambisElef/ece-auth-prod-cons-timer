/*
 *	File	: timer.h
 *
 *	Author	: Eleftheriadis Charalampos
 *
 *	Date	: 23 July 2020
 */

#ifndef PROD_CONS_TIMER_TIMER_H
#define PROD_CONS_TIMER_TIMER_H

#include <stdio.h>
#include "queue.h"

typedef struct {
    int period;
    int tasksToExecute;
    int startDelay;
    void *(*timerFcn)(void *arg);
    void *(*errorFcn)();

    Queue *queue;
    void *(*producer)(void *arg);
    int *tJobIn;
    int *tDrift;
    pthread_mutex_t *tMut;
} Timer;

void startFcn(Timer *timer, int period, int tasksToExecute, int startDelay, void *(*timerFcn)(void *arg),
        void *(*errorFcn)(), Queue *queue, void *(*producer)(void *arg), int *tJobIn, int *tDrift,
        pthread_mutex_t *tMut);
void stopFcn(Timer *timer);

void start(Timer *timer);

void startat(Timer timer, int year, int month, int day, int hour, int minute, int second);

#endif //PROD_CONS_TIMER_TIMER_H
