#ifndef HELPER_H
#define HELPER_H


#ifndef QUEUESIZE
#define QUEUESIZE 5
#endif


#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>



typedef struct {
    void *(*task)(void *);
    void *arg;
    struct timeval taskWaitingTimeToStart; // to track the time when a job/function started waiting in the queue
} WorkFunction;

typedef struct {

    WorkFunction buf[QUEUESIZE];
    long head, tail; // index positions
    int full, empty; // flags
    pthread_mutex_t *mutexQ; //for controlling access to the queue
    pthread_cond_t *notFull, *notEmpty; // for signaling when the queue is not full or not empty
} Queue;

Queue *queueInit();

void queueDelete(Queue *q);

void queueAdd(Queue *q, WorkFunction in);

void queueDel(Queue *q, WorkFunction *out);



typedef struct {
    int period;
    int tasksToExecute;
    int startDelay;
    void *(*startFcn)(void *arg);
    void *(*stopFcn)(void *arg);
    void *(*timerFcn)(void *arg);
    void *(*errorFcn)();

    Queue *queue;
    pthread_t tid;
    void *(*producer)(void *arg);
    int *timeToAddTask; // Time taken for a producer to add a job to the q
    int *driftTime;
    pthread_mutex_t *mutexTimer;
} Timer;

void timerInit(Timer *timer, int period, int tasksToExecute, void *(*startFcn)(void *arg), void *(*stopFcn)(void *arg),
        void *(*timerFcn)(void *arg), void *(*errorFcn)(), Queue *queue, void *(*producer)(void *arg), int *timeToAddTask, 
        int *driftTime, pthread_mutex_t *mutexTimer);

void startat(Timer *timer, int year, int month, int day, int hour, int minute, int second);

#endif // HELPER_H



