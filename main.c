/*
 *	File	: main.c
 *
 *	Author	: Eleftheriadis Charalampos
 *
 *	Date	: 23 July 2020
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include <pthread.h>
#include "queue.h"
#include "timer.h"

#define QUEUE_SIZE 10
#define SECONDS_TO_RUN 10

void *work(void *arg);
void *producer(void *arg);
void *consumer(void *arg);

typedef struct {
    Queue *queue;
    int totalTasks;
    int *tJobWait;
    int *tJobOut;
    int *tJobDur;
    pthread_mutex_t *tMut;
} ConsumerArguments;

// A counter for the producers to know how many jobs they have added.
int jobInCounter;

// A counter for the consumers to know when all produced elements have been consumed, so they can now quit.
int jobOutCounter;

int main () {

    // Initializes timers' period in milliseconds.
    const int period[3] = {1000, 100, 10};

    // Selects mode.
    int mode = 0;
    printf("Available Options:\n");
    printf("1 - Timer with 1 sec period\n");
    printf("2 - Timer with 0.1 sec period\n");
    printf("3 - Timer with 0.01 sec period\n");
    printf("4 - All of the above\n");
    printf("Select Mode: ");
    scanf("%d", &mode);
    if (mode!=1 && mode!=2 && mode!=3 && mode!=4) {
        printf("Nonexistent Mode Selection!\n");
        return -1;
    }

    // Calculates total number of tasks.
    int totalTasks = 0;
    if (mode == 1)
        totalTasks = SECONDS_TO_RUN * 1e3 / period[0];
    else if (mode == 2)
        totalTasks = SECONDS_TO_RUN * 1e3 / period[1];
    else if (mode == 3)
        totalTasks = SECONDS_TO_RUN * 1e3 / period[2];
    else if (mode == 4)
        totalTasks = SECONDS_TO_RUN * 1e3 / period[0] + SECONDS_TO_RUN * 1e3 / period[1] + SECONDS_TO_RUN * 1e3 / period[2];

    //! Opens files for statistics.
    // Opens file to write time taken from the moment a job is pushed to the queue until it gets popped.
    FILE *fTJobWait = fopen("tJobWait.csv", "w");
    // Opens file to write time taken for a producer to push a job.
    FILE *fTJobIn = fopen("tJobIn.csv", "w");
    // Opens file to write time taken for a consumer to pop a job.
    FILE *fTJobOut = fopen("tJobOut.csv", "w");
    // Opens file to write time taken for a consumer to execute a job.
    FILE *fTJobDur = fopen("tJobDur.csv", "w");
    // Opens file to write each producer's time drifting.
    FILE *fTDrift, *fTDrift0, *fTDrift1, *fTDrift2;
    if (mode == 4) {
        fTDrift0 = fopen("tDrift0.csv", "w");
        fTDrift1 = fopen("tDrift1.csv", "w");
        fTDrift2 = fopen("tDrift2.csv", "w");
    }
    else
        fTDrift = fopen("tDrift.csv", "w");

    // Initializes random number seed.
    srand(time(NULL));

    for (int conNum=1; conNum<3; conNum*=2) {

        // Prints a message.
        printf("#Cons=%d Started.\n",conNum);

        // jobInCounter and jobOutCounter begin from -1 each time.
        jobInCounter = -1;
        jobOutCounter = -1;

        //! Allocates memory for statistics with cells equal to the expected production.
        // tJobWait: Time taken from the moment a job is pushed to the queue until it gets popped.
        int *tJobWait = (int *)malloc(totalTasks*sizeof(int));
        // tJobIn: Time taken for a producer to push a job.
        int *tJobIn = (int *)malloc(totalTasks*sizeof(int));
        // tJobOut: Time taken for a consumer to pop a job.
        int *tJobOut = (int *)malloc(totalTasks*sizeof(int));
        // tJobDur: Time taken for a consumer to execute a job.
        int *tJobDur = (int *)malloc(totalTasks*sizeof(int));
        // tDrift: Producer's time drifting.
        int *tDrift, *tDrift0, *tDrift1, *tDrift2;
        if (mode == 4) {
            tDrift0 = (int *)malloc(SECONDS_TO_RUN*1e3/period[0] * sizeof(int));
            tDrift1 = (int *)malloc(SECONDS_TO_RUN*1e3/period[1] * sizeof(int));
            tDrift2 = (int *)malloc(SECONDS_TO_RUN*1e3/period[2] * sizeof(int));
        }
        else
            tDrift = (int *)malloc(totalTasks*sizeof(int));

        // Initializes Queue.
        Queue *fifo;
        fifo = queueInit(QUEUE_SIZE);
        if (fifo ==  NULL) {
            fprintf (stderr, "main: Queue Init failed.\n");
            exit (1);
        }

        // Initializes consumer threads arguments.
        ConsumerArguments *consArgs = (ConsumerArguments *)malloc(sizeof(ConsumerArguments));
        consArgs->queue = fifo;
        consArgs->totalTasks = totalTasks;
        consArgs->tJobWait = tJobWait;
        consArgs->tJobOut = tJobOut;
        consArgs->tJobDur = tJobDur;
        consArgs->tMut = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(consArgs->tMut, NULL);
        // Creates consumer threads.
        pthread_t con[conNum];
        for (int i=0; i<conNum; i++)
            pthread_create(&con[i], NULL, consumer, consArgs);

        // Creates timer.
        Timer *timer;
        if (mode == 1) {
            timer = (Timer *)malloc(sizeof(Timer));
            startFcn(timer, period[0], SECONDS_TO_RUN*1e3/period[0], 0, work, fifo, producer, tJobIn, tDrift);
            start(timer);
        }
        else if (mode== 2) {
            timer = (Timer *)malloc(sizeof(Timer));
            startFcn(timer, period[1], SECONDS_TO_RUN*1e3/period[1], 0, work, fifo, producer, tJobIn, tDrift);
            start(timer);
        }
        else if (mode== 3) {
            timer = (Timer *)malloc(sizeof(Timer));
            startFcn(timer, period[2], SECONDS_TO_RUN*1e3/period[2], 0, work, fifo, producer, tJobIn, tDrift);
            start(timer);
        }
        else if (mode== 4) {
            timer = (Timer *)malloc(3 * sizeof(Timer));
            startFcn(&timer[0], period[0], SECONDS_TO_RUN*1e3/period[0], 0, work, fifo, producer, tJobIn, tDrift0);
            startFcn(&timer[1], period[1], SECONDS_TO_RUN*1e3/period[1], 0, work, fifo, producer, tJobIn, tDrift1);
            startFcn(&timer[2], period[2], SECONDS_TO_RUN*1e3/period[2], 0, work, fifo, producer, tJobIn, tDrift2);
            start(&timer[0]);
            start(&timer[1]);
            start(&timer[2]);
        }

        // Waits for jobs to finish execution.
        while(jobOutCounter != totalTasks-1);

        // Signals the consumer that queue is not empty, so they can quit safely.
        pthread_cond_signal(timer->queue->notEmpty);

        // Waits for threads to finish.
        for (int i=0; i<conNum; i++)
            pthread_join (con[i], NULL);

        //! Saves statistics.
        // Writes tJobWait to file. The number of row represents the number of consumers of the test.
        for (int i=0; i<totalTasks; i++)
            fprintf(fTJobWait, "%d,", tJobWait[i]);
        fprintf(fTJobWait, "\n");
        // Writes tJobIn to file. The number of row represents the number of consumers of the test.
        for (int i=0; i<totalTasks; i++)
            fprintf(fTJobIn, "%d,", tJobIn[i]);
        fprintf(fTJobIn, "\n");
        // Writes tJobOut to file. The number of row represents the number of consumers of the test.
        for (int i=0; i<totalTasks; i++)
            fprintf(fTJobOut, "%d,", tJobOut[i]);
        fprintf(fTJobOut, "\n");
        // Writes tJobDur to file. The number of row represents the number of consumers of the test.
        for (int i=0; i<totalTasks; i++)
            fprintf(fTJobDur, "%d,", tJobDur[i]);
        fprintf(fTJobDur, "\n");
        // Writes tDrift to file. The number of row represents the number of consumers of the test.
        if (mode == 4) {
            for (int i=0; i<SECONDS_TO_RUN*1e3/period[0]; i++)
                fprintf(fTDrift0, "%d,", tDrift0[i]);
            fprintf(fTDrift0, "\n");
            for (int i=0; i<SECONDS_TO_RUN*1e3/period[1]; i++)
                fprintf(fTDrift1, "%d,", tDrift1[i]);
            fprintf(fTDrift1, "\n");
            for (int i=0; i<SECONDS_TO_RUN*1e3/period[2]; i++)
                fprintf(fTDrift2, "%d,", tDrift2[i]);
            fprintf(fTDrift2, "\n");
        }
        else {
            for (int i=0; i<totalTasks; i++)
                fprintf(fTDrift, "%d,", tDrift[i]);
            fprintf(fTDrift, "\n");
        }

        //! Cleans up.
        // Deletes Queue.
        queueDelete(fifo);

        // Stops Timer.
        stopFcn(timer);

        // Releases memory.
        free(tJobWait);
        free(tJobIn);
        free(tJobOut);
        free(tJobDur);
        if (mode == 4) {
            free(tDrift0);
            free(tDrift1);
            free(tDrift2);
        }
        else
            free(tDrift);

        // Sleeps for 100ms before next iteration.
        sleep(0.1);

    }

    // Closes files.
    fclose(fTJobWait);
    fclose(fTJobIn);
    fclose(fTJobOut);
    fclose(fTJobDur);
    if (mode == 4) {
        fclose(fTDrift0);
        fclose(fTDrift1);
        fclose(fTDrift2);
    }
    else
        fclose(fTDrift);

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

    struct timeval tJobInStart, tJobInEnd, tProdExec[2];
    int driftCounter = -1;

    for (int i=0; i<timer->tasksToExecute; i++) {
        // Creates the work function arguments. k is the number of them.
        tProdExec[0] = tProdExec[1];
        gettimeofday(&tProdExec[1], NULL);
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
        gettimeofday(&tJobInStart, NULL);
        pthread_mutex_lock(timer->queue->mut);

        // This producer is going to add an element, so the jobInCounter is increased.
        jobInCounter++;

        while (timer->queue->full) {
            //printf ("producer: queue FULL.\n");
            pthread_cond_wait(timer->queue->notFull, timer->queue->mut);
        }
        gettimeofday(&in.tJobWaitStart, NULL);
        queueAdd(timer->queue, in);
        gettimeofday(&tJobInEnd, NULL);

        // Calculates tJobIn.
        int tJobIn = (tJobInEnd.tv_sec-tJobInStart.tv_sec)*1e6 + tJobInEnd.tv_usec-tJobInStart.tv_usec;
        timer->tJobIn[jobInCounter] = tJobIn;

        // Critical section ends.
        pthread_mutex_unlock(timer->queue->mut);

        // Signals the consumer that queue is not empty.
        pthread_cond_signal(timer->queue->notEmpty);

        // Skip time drifting logic for first iteration.
        if (i==0) {
            usleep(timer->period * 1e3);
            continue;
        }

        // Logic to face time drifting.
        int tDrift = (tProdExec[1].tv_sec-tProdExec[0].tv_sec)*1e6 + tProdExec[1].tv_usec-tProdExec[0].tv_usec - timer->period*1e3;
        timer->tDrift[++driftCounter] = tDrift;
        usleep(timer->period * 1e3 - tDrift);
    }

    return(NULL);
}

void *consumer(void *arg) {
    ConsumerArguments *consArgs = (ConsumerArguments *)arg;

    struct timeval tJobOutStart, tJobOutEnd, tJobDurStart, tJobDurEnd;
    WorkFunction out;

    while (1) {
        // Critical section begins.
        gettimeofday(&tJobOutStart, NULL);
        pthread_mutex_lock(consArgs->queue->mut);

        while (consArgs->queue->empty) {
            //printf ("consumer: Queue EMPTY.\n");
            pthread_cond_wait(consArgs->queue->notEmpty, consArgs->queue->mut);
        }

        // Checks if the number of consumed elements has matched the production. If yes, then this consumer exits.
        if (jobOutCounter == consArgs->totalTasks-1) {
            pthread_mutex_unlock(consArgs->queue->mut);
            break;
        }

        // Pops job from queue.
        queueDel (consArgs->queue, &out);
        gettimeofday(&tJobOutEnd, NULL);

        // Critical section ends.
        pthread_mutex_unlock(consArgs->queue->mut);

        // Signals to producer that Queue is not full.
        pthread_cond_signal(consArgs->queue->notFull);

        // Executes work outside the critical section.
        gettimeofday(&tJobDurStart, NULL);
        out.work(out.arg);
        gettimeofday(&tJobDurEnd, NULL);

        // Critical section starts to write shared time statistics.
        pthread_mutex_lock(consArgs->tMut);

        jobOutCounter++;

        // Calculates tJobWait.
        int tJobWait = (tJobOutEnd.tv_sec-out.tJobWaitStart.tv_sec)*1e6 + tJobOutEnd.tv_usec-out.tJobWaitStart.tv_usec;
        consArgs->tJobWait[jobOutCounter] = tJobWait;
        // Calculates tJobOut.
        int tJobOut = (tJobOutEnd.tv_sec-tJobOutStart.tv_sec)*1e6 + tJobOutEnd.tv_usec-tJobOutStart.tv_usec;
        consArgs->tJobOut[jobOutCounter] = tJobOut;
        // Calculates tJobDur.
        int tJobDur = (tJobDurEnd.tv_sec-tJobDurStart.tv_sec)*1e6 + tJobDurEnd.tv_usec-tJobDurStart.tv_usec;
        consArgs->tJobDur[jobOutCounter] = tJobDur;

        // Critical section ends.
        pthread_mutex_unlock(consArgs->tMut);
    }

    return(NULL);
}

