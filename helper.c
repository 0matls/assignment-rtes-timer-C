#include "helper.h"


Queue *queueInit() {
    Queue *q;

    q = (Queue *)malloc(sizeof(Queue));
    if (q == NULL) return NULL;

    q->empty = 1;
    q->full = 0;
    q->head = 0;
    q->tail = 0;

    q->mutexQ = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(q->mutexQ, NULL);
    q->notFull = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    pthread_cond_init(q->notFull, NULL);
    q->notEmpty = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    pthread_cond_init(q->notEmpty, NULL);

    return q;
}


void queueAdd(Queue *q, WorkFunction in) {
    q->buf[q->tail] = in;
    q->tail++;

    if (q->tail == QUEUESIZE)
        q->tail = 0;
    if (q->tail == q->head)
        q->full = 1;
    q->empty = 0;
}

void queueDelete(Queue *q) {
    pthread_mutex_destroy (q->mutexQ);
    free (q->mutexQ);
    pthread_cond_destroy (q->notFull);
    free (q->notFull);
    pthread_cond_destroy (q->notEmpty);
    free (q->notEmpty);

    free (q);
}


void queueDel(Queue *q, WorkFunction *out) {
    *out = q->buf[q->head];

    q->head++;
    if (q->head == QUEUESIZE)
        q->head = 0;
    if (q->head == q->tail)
        q->empty = 1;
    q->full = 0;
}




void timerInit(Timer *timer, int period, int tasksToExecute, void *(*startFcn)(void *arg), void *(*stopFcn)(void *arg),
        void *(*timerFcn)(void *arg), void *(*errorFcn)(), Queue *queue, void *(*producer)(void *arg), int *timeToAddTask, 
        int *driftTime, pthread_mutex_t *mutexTimer){
    timer->period = period;
    timer->tasksToExecute = tasksToExecute;
    timer->startDelay = 0.1;
    timer->startFcn = startFcn;
    timer->stopFcn = stopFcn;
    timer->timerFcn = timerFcn;
    timer->errorFcn = errorFcn;

    timer->queue = queue;
    timer->producer = producer;
    timer->timeToAddTask = timeToAddTask;
    timer->driftTime = driftTime;
    timer->mutexTimer = mutexTimer;
}


void startat(Timer *timer, int year, int month, int day, int hour, int minute, int second) {

    time_t currentTime = time(NULL);

    // to get the exact time
    struct tm startingTime;
    startingTime.tm_year = year;
    startingTime.tm_mon = month;
    startingTime.tm_mday = day;
    startingTime.tm_hour = hour;
    startingTime.tm_min = minute;
    startingTime.tm_sec = second;

    int delay = (int) difftime(currentTime, mktime(&startingTime));

    if (delay<0)
        delay = 0;
    timer->startDelay = delay;
    pthread_create(&timer->tid, NULL, timer->producer, timer);
}


