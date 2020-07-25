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

#define QUEUE_SIZE 10
#define TASKS_TO_EXECUTE 10


void *work(void *arg);
void *producer(void *arg);
void *consumer(void *arg);

typedef struct {
    Queue *queue;
    int totalTasks;
} ConsumerArguments;

// A counter for the consumers to know when all produced elements have been consumed, so they can now quit.
int outCounter;

int main () {

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
        totalTasks = TASKS_TO_EXECUTE;
    else if (mode == 2)
        totalTasks = TASKS_TO_EXECUTE*10;
    else if (mode == 3)
        totalTasks = TASKS_TO_EXECUTE*100;
    else if (mode == 4)
        totalTasks = TASKS_TO_EXECUTE + TASKS_TO_EXECUTE*10 + TASKS_TO_EXECUTE*100;

    // Initializes random number seed.
    srand(time(NULL));

    // Opens file.
    FILE *fp;
    fp = fopen("test.csv", "w");

    for (int conNum=1; conNum<3; conNum*=2) {

        // Prints a message.
        printf("#Cons=%d Started.\n",conNum);

        // outCounter begins from -1 each time.
        outCounter = -1;

        // Allocates array with cells equal to the expected production.
        // Each cell will contain the in-Queue waiting time of each produced element.
        int *toWrite = (int *)malloc(totalTasks*sizeof(int));

        // Initializes Queue.
        Queue *fifo;
        fifo = queueInit(QUEUE_SIZE, toWrite);
        if (fifo ==  NULL) {
            fprintf (stderr, "main: Queue Init failed.\n");
            exit (1);
        }

        // Creates consumer threads.
        pthread_t con[conNum];
        ConsumerArguments *consArgs = (ConsumerArguments *)malloc(sizeof(ConsumerArguments));
        consArgs->queue = fifo;
        consArgs->totalTasks = totalTasks;
        for (int i=0; i<conNum; i++)
            pthread_create(&con[i], NULL, consumer, consArgs);

        // Creates timer.
        if (mode == 1) {
            Timer *timer = (Timer *)malloc(sizeof(Timer));
            startFcn(timer, 1000, TASKS_TO_EXECUTE, 0, work, fifo, producer);
            start(timer);
        }
        else if (mode== 2) {
            Timer *timer = (Timer *)malloc(sizeof(Timer));
            startFcn(timer, 100, TASKS_TO_EXECUTE*10, 0, work, fifo, producer);
            start(timer);
        }
        else if (mode== 3) {
            Timer *timer = (Timer *)malloc(sizeof(Timer));
            startFcn(timer, 10, TASKS_TO_EXECUTE*100, 0, work, fifo, producer);
            start(timer);
        }
        else if (mode== 4) {
            Timer *timer = (Timer *)malloc(3 * sizeof(Timer));
            startFcn(&timer[0], 1000, TASKS_TO_EXECUTE, 0, work, fifo, producer);
            startFcn(&timer[1], 100, TASKS_TO_EXECUTE*10, 0, work, fifo, producer);
            startFcn(&timer[2], 10, TASKS_TO_EXECUTE*100, 0, work, fifo, producer);
            start(&timer[0]);
            start(&timer[1]);
            start(&timer[2]);
        }

        // Waits for threads to finish.
        for (int i=0; i<conNum; i++)
            pthread_join (con[i], NULL);

        // Deletes Queue.
        queueDelete (fifo);

        // Writes results to file. The number of row represents the number of consumers of the test.
        for (int i=0; i<totalTasks; i++)
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

    ConsumerArguments *consArgs = (ConsumerArguments *)arg;
    fifo = (Queue *)consArgs->queue;

    while (1) {
        // Critical section begins.
        pthread_mutex_lock (fifo->mut);

        // Checks if the number of consumed elements has matched the production. If yes, then this consumer exits.
        if (outCounter == consArgs->totalTasks-1) {
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

