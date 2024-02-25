#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include <pthread.h>
#include "helper.h"


void *producer(void *arg);
void *consumer(void *arg);
void *task(void *arg);
void *startTimer(void *arg);
void *stopTimer(void *arg);
void *error();
void writeData(int size, const char *name, int *data);


int addedTasks = 0; // Counts the number of jobs that the producers have added to the q
int finishedTasks = 0; // number of jobs that the consumers have executed from the q
int errors = 0;
int quitSignal = 0;

typedef struct {
    pthread_mutex_t *mutexTimer;
    Queue *queue;

    int *quitSignal; // Flag for the consumers to quit when everything is done
    int *taskWaitingTime;  // Time taken from when a job is added to the queue to when it's removed
    int *taskExecutionTime;  // Time taken for a consumer to execute a job
    int totalTasks;
} Utilities;


int main(int argc, char *argv[]) {
    
    // Parse the caseSelected from the command-line argument
    int caseSelected = atoi(argv[1]);

    if (caseSelected < 1 || caseSelected > 4) {
        printf("Invalid case. Please select a case from 1 to 4.\n");
        return -1;
    }
	
    srand(time(NULL));

    // total running time 
    unsigned int secondsToRun = 1800; // 3600 to get an hour

    unsigned int totalTasks  = 0;
    int period = 0;

    if (caseSelected == 1){
        period = 1000;
        totalTasks = secondsToRun;} // period = 1000 ms
    else if (caseSelected == 2){
        period = 100;
        totalTasks = secondsToRun * (int)10;} // period = 100 ms
    else if (caseSelected == 3){
        period = 10;
        totalTasks = secondsToRun * (int)100;}// period = 10 ms
    else if (caseSelected == 4){
        totalTasks = secondsToRun * (int)(1 + 10 + 100);}

	printf("Total Jobs: %d\n", totalTasks);


    // Initialize FIFO queue
    Queue *fifoQ;
    fifoQ = queueInit();
    if (fifoQ == NULL) {
        fprintf(stderr, "Queue Initialization failed.\n");
        return -1;
    }
    
    int *taskWaitingTime = (int *)malloc(totalTasks * sizeof(int));
    int *timeToAddTask = (int *)malloc(totalTasks * sizeof(int));  // Time taken for a producer to add a job to the q
    int *taskExecutionTime = (int *)malloc(totalTasks * sizeof(int));

    // drifting times
    int *driftingTime, *driftingTimeP1, *driftingTimeP2, *driftingTimeP3;
    if (caseSelected == 1 || caseSelected == 2 || caseSelected ==3 ) {
        driftingTime = (int *)malloc(totalTasks * sizeof(int));
    }
    else if (caseSelected == 4) {
        driftingTimeP1 = (int *)malloc(secondsToRun * (int)1* sizeof(int));
        driftingTimeP2 = (int *)malloc(secondsToRun * (int)10* sizeof(int));
        driftingTimeP3 = (int *)malloc(secondsToRun * (int)100* sizeof(int));
    }

    unsigned int consumersNum  = 4; // defines the number of consumers

    Utilities *utils = (Utilities *)malloc(sizeof(Utilities));
    utils->queue = fifoQ;
    utils->totalTasks = totalTasks;
    utils->taskWaitingTime = taskWaitingTime;
    utils->taskExecutionTime = taskExecutionTime;
    utils->mutexTimer = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(utils->mutexTimer, NULL);

    // Create consumer threads and timers
    pthread_t con[consumersNum];
    for (int i=0; i<consumersNum; i++)
        pthread_create(&con[i], NULL, consumer, utils);

    Timer *timer;
    pthread_mutex_t *mutexTimer = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutexTimer, NULL);


    if (caseSelected == 1 || caseSelected == 2 || caseSelected == 3) {
        timer = (Timer *)malloc(sizeof(Timer));
        timerInit(timer, period, secondsToRun * (int)1e3 / period, startTimer, stopTimer,
                task, error, fifoQ, producer, timeToAddTask, driftingTime, mutexTimer);
        timer->startFcn((void *) timer);
    }
    else if (caseSelected == 4) {
        timer = (Timer *)malloc(3 * sizeof(Timer));
        timerInit(&timer[0], (int)1000, secondsToRun * (int)1, startTimer,
                stopTimer, task, error, fifoQ, producer, timeToAddTask, driftingTimeP1, mutexTimer);
        timerInit(&timer[1],(int)100, secondsToRun * (int)10, startTimer,
                stopTimer, task, error, fifoQ, producer, timeToAddTask, driftingTimeP2, mutexTimer);
        timerInit(&timer[2], (int)10, secondsToRun * (int)100, startTimer,
                stopTimer, task, error, fifoQ, producer, timeToAddTask, driftingTimeP3, mutexTimer);

        timer[0].startFcn((void *) &timer[0]);
        timer[1].startFcn((void *) &timer[1]);
        timer[2].startFcn((void *) &timer[2]);
    }

    // wait for timer(s) to finish
    if (caseSelected == 4) {
        pthread_join(timer[0].tid, NULL);
        pthread_join(timer[1].tid, NULL);
        pthread_join(timer[2].tid, NULL);
    }
    else
        pthread_join(timer->tid, NULL);

    // flag is set to 1 to help the consumers to exit when all the task are done
    quitSignal = 1;


    // Signal the consumers that queue is not empty, so they can quit safely
    pthread_cond_broadcast(fifoQ->notEmpty);

    // Wait for threads to finish
    for (int i=0; i<consumersNum; i++)
        pthread_join(con[i], NULL);


    printf("Consumer - producer finished. Start writing statistics.\n");

    struct timeval timestamp;
    gettimeofday(&timestamp, NULL);

    char taskWaitingTimeFile[80];
    sprintf(taskWaitingTimeFile, "taskWaitingTime_%d.csv", (int)timestamp.tv_sec);
    char timeToAddTaskFile[80];
    sprintf(timeToAddTaskFile, "timeToAddTask_%d.csv", (int)timestamp.tv_sec);
    char taskExecutionTimeFile[80];
    sprintf(taskExecutionTimeFile, "taskExecutionTime_%d.csv", (int)timestamp.tv_sec);


    // writeData((int)finishedTasks, "taskWaitingTime.csv", taskWaitingTime);
    writeData((int)finishedTasks, taskWaitingTimeFile, taskWaitingTime);
    writeData((int)finishedTasks, timeToAddTaskFile, timeToAddTask);
    writeData((int)finishedTasks, taskExecutionTimeFile, taskExecutionTime);
    if (caseSelected == 4) {
        char driftingTime1000File[80];
        sprintf(driftingTime1000File, "driftingTime1000_%d.csv", (int)timestamp.tv_sec);
        writeData(secondsToRun - 1, driftingTime1000File, driftingTimeP1);
        char driftingTime100File[80];
        sprintf(driftingTime100File, "driftingTime100_%d.csv", (int)timestamp.tv_sec);
        writeData(secondsToRun * (int)10 - 1, driftingTime100File, driftingTimeP2);
        char driftingTime10File[80];
        sprintf(driftingTime10File, "driftingTime10_%d.csv", (int)timestamp.tv_sec);
        writeData(secondsToRun * (int)100 - 1, driftingTime10File, driftingTimeP3);               
    }
    else {
        char driftingTimeFile[80];
        sprintf(driftingTimeFile, "driftingTime_%d.csv", (int)timestamp.tv_sec);
        writeData(finishedTasks-1, driftingTimeFile, driftingTime);
    }
        

    printf("Writing statistics finished. Time for a clean up.\n");


    // Free the occupied space
    free(taskWaitingTime);
    free(timeToAddTask);
    free(taskExecutionTime);
    if (caseSelected == 4) {
        free(driftingTimeP1);
        free(driftingTimeP2);
        free(driftingTimeP3);
    }
    else
        free(driftingTime);
    free(timer);
    queueDelete(fifoQ);
    pthread_mutex_destroy (utils->mutexTimer);
    free(utils->mutexTimer);
    free(utils);
    pthread_mutex_destroy (mutexTimer);
    free(mutexTimer);
    printf("errors are: %d \n", errors);

    return 0;
}


void *producer(void *arg) {

    int totalDriftTime = 0;
    int driftCounter = 0;

    Timer *timer = (Timer *)arg;
    // Initial Delay
    sleep(timer->startDelay);
    
    struct timeval tProdExecStart, tProdExecEnd, temp;
    
    struct timeval timeToAddTaskStart, timeToAddTaskEnd;
    
    for (int i=0; i<timer->tasksToExecute; i++) {
        printf("task %d\n", i);
        // Initialize timers for drifting to avoid multiple excecution of code
        // (shift of information by one element)
        gettimeofday(&temp, NULL);
        tProdExecStart = (i==0) ? temp : tProdExecEnd; 
        tProdExecEnd = temp;

        gettimeofday(&timeToAddTaskStart, NULL);
        
        // Create a matrix userData with random dimensions between 50 and 100
        int rows = (rand() % 51) + 50;

        int *userData = (int *)malloc(rows * sizeof(int *));
        for (int i = 0; i < rows; i++) {
            userData[i] = rand() % 100;  // Fill matrix with random values [0, 99]
        }
        // set the information for task 
        userData[0] = rows;

        // Create the task that will be added to the Q
        WorkFunction product;
        product.task = timer->timerFcn;
        product.arg = userData;

        pthread_mutex_lock(timer->queue->mutexQ);                   // mutex (Q) LOCK 

        
        if (timer->queue->full) {  // Queue is full, so job is lost and the error function is called

            pthread_mutex_unlock(timer->queue->mutexQ);             // mutex (Q) UNLOCK (case 1: because the Q is full)

            // signal to consumers that queue is not empty
            pthread_cond_signal(timer->queue->notEmpty);

            // call error function because the Q is full 
            pthread_mutex_lock(timer->mutexTimer);                  // mutex (error) LOCK 
            timer->errorFcn();
            pthread_mutex_unlock(timer->mutexTimer);                // mutex (error) UNLOCK 
        }
        else {         // Q is not full, so the task is added to it
            gettimeofday(&product.taskWaitingTimeToStart, NULL);
            queueAdd(timer->queue, product);
            gettimeofday(&timeToAddTaskEnd, NULL);

            pthread_mutex_unlock(timer->queue->mutexQ);             // mutex (Q) UNLOCK (case 2: normal execution, addition of task to the Q)

            // signal to consumers that queue is not empty
            pthread_cond_signal(timer->queue->notEmpty);

            pthread_mutex_lock(timer->mutexTimer);                   // mutex (stats) LOCK 

            // timeToAddTask to the Q
            timer->timeToAddTask[addedTasks++] = (timeToAddTaskEnd.tv_sec-timeToAddTaskStart.tv_sec)*(int)1e6 + timeToAddTaskEnd.tv_usec-timeToAddTaskStart.tv_usec;;

            pthread_mutex_unlock(timer->mutexTimer);                 // mutex (stats) UNLOCK 
        }

        // no drift on first iteration
        if (i==0) {
            usleep(timer->period*1e3);
            continue;
        }

  
        int currentDriftTime = (tProdExecEnd.tv_sec-tProdExecStart.tv_sec)*(int)1e6 + tProdExecEnd.tv_usec-tProdExecStart.tv_usec - timer->period*1e3;
        totalDriftTime += currentDriftTime;
      
        // face time drifting
        if (totalDriftTime>timer->period*(int)1e3)
            currentDriftTime = timer->period*(int)1e3;
        else
            currentDriftTime = totalDriftTime;

        timer->driftTime[driftCounter] = totalDriftTime;
        driftCounter+=1;
        usleep(timer->period*(int)1e3 - currentDriftTime);
    }

    // stop the timer
    timer->stopFcn((void *) &timer->tasksToExecute);
    return NULL;
}



void *consumer(void *arg) {

    struct timeval taskWaitingTimeEnd, taskStartingTime, taskFinishingTime;
    WorkFunction taskDone;
    Utilities *utils = (Utilities *)arg;

    while (1) {
        // This is necessary to avoid multiple consumers reading the queue at the same time
        // However, this may lead to a problem in a consumer heavy scenario that the producers
        // will be locked out of queue before adding all their tasks.

        pthread_mutex_lock(utils->queue->mutexQ);             // mutex (q) LOCK

        while (utils->queue->empty) { 
            // empty queue
            pthread_cond_wait(utils->queue->notEmpty, utils->queue->mutexQ);

            // Check flag to quit when signaled
            if (quitSignal) {
                pthread_mutex_unlock(utils->queue->mutexQ);
                return NULL;
            }
        }

        // takes task from queue
        queueDel(utils->queue, &taskDone);
        gettimeofday(&taskWaitingTimeEnd, NULL); // keep the value of the end of the task's waiting time

        pthread_mutex_unlock(utils->queue->mutexQ);           // mutex (q) UNLOCK

        // Signals the producer that the q is not full
        pthread_cond_signal(utils->queue->notFull);

        // Executes task outside the critical section.
        gettimeofday(&taskStartingTime, NULL);
        taskDone.task(taskDone.arg);
        gettimeofday(&taskFinishingTime, NULL);
        // Frees task function arguments allocated dynamically from the producer and result allocated from the task function.
        free(taskDone.arg);
    
        pthread_mutex_lock(utils->mutexTimer);                   // mutex (stats) LOCK

        // Calculates taskWaitingTime.
        int taskWaitingTime = (taskWaitingTimeEnd.tv_sec-taskDone.taskWaitingTimeToStart.tv_sec)*(int)1e6 + taskWaitingTimeEnd.tv_usec-taskDone.taskWaitingTimeToStart.tv_usec;
        utils->taskWaitingTime[finishedTasks] = taskWaitingTime;
        // Calculates taskExecutionTime.
        int taskExecutionTime = (taskFinishingTime.tv_sec-taskStartingTime.tv_sec)*(int)1e6 + taskFinishingTime.tv_usec-taskStartingTime.tv_usec;
        utils->taskExecutionTime[finishedTasks++] = taskExecutionTime;

        pthread_mutex_unlock(utils->mutexTimer);                // mutex (stats) UNLOCK // O
    }
}


void *task(void *arg) {    
    int *userData = (int *)arg;
    int rows = userData[0];
    
    int result = 0;
    // Calculate userData^2 = userData^T * userData
    for (int i = 0; i < rows; i++) {
        result += userData[i] * userData[i];
    }

    return NULL;
}


void *stopTimer(void *arg) {
    int *tasksToExecute = (int *) arg;
    printf("Timer executed %d tasks and stopped. \n", *tasksToExecute);
    return NULL;
}

// Initialize timer by adding it to the thread and informing the user
void *startTimer(void *arg) {
    Timer *timer = (Timer *) arg;
    pthread_create(&timer->tid, NULL, timer->producer, timer);
    printf("Timer with %d tasks execute started. \n", timer->tasksToExecute);
    return NULL;
}

void writeData(int size, const char *name, int *data) {
    FILE *file = fopen(name, "w");
    if (file == NULL){
        printf("Failed to open file.");
        return;
    }
    for (int i=0; i<size; i++)
        fprintf(file, "%d\n", data[i]);
    fclose(file);    
}

void *error() {
    printf("Error! \n");
    errors++;
    return NULL;
}
