/*
 *	File	: main.c
 *
 *	Author	: Eleftheriadis Charalampos
 *
 *	Date	: 23 July 2020
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include "queue.h"
#include "timer.h"

#define QUEUESIZE 10
#define TASKS1 10


void *work(void *arg);
void *producer(void *arg);
void *consumer(void *arg);

// A counter for the consumers to know when all produced elements have been consumed, so they can now quit.
int outCounter;

int main () {

    // Initializes random number seed.
    srand(time(NULL));

    // Opens file.
    FILE *fp;
    fp = fopen("test.csv", "w");

    for (int conNum=1; conNum<2; conNum*=2) {

        // Prints a message.
        printf("#Cons=%d Started.\n",conNum);

        // outCounter begins from -1 each time.
        outCounter = -1;

        // Allocates array with cells equal to the expected production.
        // Each cell will contain the in-Queue waiting time of each produced element.
        int *toWrite = (int *)malloc(TASKS1*sizeof(int));

        // Initializes Queue.
        Queue *fifo;
        fifo = queueInit(QUEUESIZE, toWrite);
        if (fifo ==  NULL) {
            fprintf (stderr, "main: Queue Init failed.\n");
            exit (1);
        }

        // Creates consumer threads.
        pthread_t con[conNum];
        for (int i=0; i<conNum; i++)
            pthread_create(&con[i], NULL, consumer, fifo);

        // Creates timer.
        Timer *timer = (Timer *)malloc(sizeof(Timer));
        startFcn(timer, 1000, TASKS1, 0, work, fifo, producer);
        start(timer);

        //! NEEDS TO CHANGE
        // Waits for threads to finish.
        for (int i=0; i<conNum; i++)
            pthread_join (con[i], NULL);

        // Deletes Queue.
        queueDelete (fifo);

        // Writes results to file. The number of row represents the number of consumers of the test.
        for (int i=0; i<TASKS1; i++)
            fprintf(fp, "%d,", toWrite[i]);
        fprintf(fp, "\n");

        // Releases memory.
        free(toWrite);

        // Sleeps for 100ms before next iteration.
        sleep(0.1);

    }

    // Closes file.
    fclose(fp);

    return 0;
}

void *work(void *arg) {
    int *a = (int *)arg;
    double r = 0;
    for (int i=0; i<a[0]; i++)
        r += sin((double)a[i+1]);

    // Prints result to screen.
    //printf("%f\n",r);
}

void *producer(void *arg) {
    Timer *timer = (Timer *)arg;

    struct timeval start, end;

    for (int i=0; i<timer->tasksToExecute; i++) {

        gettimeofday(&start, NULL);

        // Creates the work function arguments. k is the number of them.
        int k = (rand() % 101) + 100;
        int *a = (int *)malloc((k+1)*sizeof(int));
        a[0] = k;
        for (int j=0; j<k; j++)
            a[j+1] = k+j;

        // Creates the element that will be added to the queue.
        WorkFunction in;
        in.work = timer->timerFcn;
        in.arg = a;

        // Critical section begins.
        pthread_mutex_lock (timer->queue->mut);

        while (timer->queue->full) {
            //printf ("producer: queue FULL.\n");
            pthread_cond_wait (timer->queue->notFull, timer->queue->mut);
        }
        gettimeofday(&in.start, NULL);
        queueAdd (timer->queue, in);

        // Critical section ends.
        pthread_mutex_unlock (timer->queue->mut);

        // Signals the consumer that queue is not empty.
        pthread_cond_signal (timer->queue->notEmpty);

        // Logic to face time drifting.
        gettimeofday(&end, NULL);
        usleep(timer->period * 1e3 - (int)((end.tv_sec-start.tv_sec)*1e6 + (end.tv_usec-start.tv_usec)));
    }

    return(NULL);
}

void *consumer(void *arg) {
    Queue *fifo;
    WorkFunction out;
    struct timeval end;

    fifo = (Queue *)arg;

    while (1) {
        // Critical section begins.
        pthread_mutex_lock (fifo->mut);

        // Checks if the number of consumed elements has matched the production. If yes, then this consumer exits.
        if (outCounter == TASKS1-1) {
            pthread_mutex_unlock (fifo->mut);
            break;
        }

        // This consumer is going to consume an element, so the outCounter is increased.
        outCounter++;

        while (fifo->empty) {
            //printf ("consumer: Queue EMPTY.\n");
            pthread_cond_wait (fifo->notEmpty, fifo->mut);
        }
        queueDel (fifo, &out);
        gettimeofday(&end, NULL);
        fifo->toWrite[outCounter] = (int)((end.tv_sec-out.start.tv_sec)*1e6 + (end.tv_usec-out.start.tv_usec));

        // Critical section ends.
        pthread_mutex_unlock (fifo->mut);

        // Signals to producer that Queue is not full.
        pthread_cond_signal (fifo->notFull);

        // Executes work outside the critical section.
        out.work(out.arg);

    }

    return(NULL);
}

