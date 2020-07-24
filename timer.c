#include "timer.h"

void startFcn(Timer *timer, int period, int tasksToExecute, int startDelay, void *(*timerFcn)(void *arg), Queue *queue, void *(*producer)(void *arg)) {
    timer->period = period;
    timer->tasksToExecute = tasksToExecute;
    timer->startDelay = startDelay;
    timer->timerFcn = timerFcn;

    timer->queue = queue;
    timer->producer = producer;
}

void stopFcn(Timer *timer) {
    free(timer);
}

void start(Timer timer) {
    pthread_t pro;
    pthread_create(&pro, NULL, timer.producer, &timer);
}

