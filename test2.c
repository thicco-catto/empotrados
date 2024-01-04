#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/time.h>

#define SIGNAL_TIMER_A SIGRTMIN+1
#define SIGNAL_TIMER_B SIGRTMIN+2
#define SIGNAL_A_CALL_C SIGRTMIN+3
#define SIGNAL_B_CALL_C SIGRTMIN+4

#define PERIOD_A 1
#define INCREMENT_A 1
#define MULTIPLE_A 10

#define PERIOD_B 2
#define INCREMENT_B 2
#define MULTIPLE_B 5

struct Data {
    int contadorA;
    int contadorB;
    pthread_mutex_t mutexA;
    pthread_mutex_t mutexB;
};

void* TareaA(void* args) {
    //Get data
    struct Data* data = args;

    //Init signal
    struct sigevent sigev;
    sigset_t sigset;
    int signum;

    sigev.sigev_notify = SIGEV_SIGNAL;
    sigev.sigev_signo = SIGNAL_TIMER_A;

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGNAL_TIMER_A);

    //Init timer
    timer_t timer;
    struct itimerspec timerspec;

    timerspec.it_interval.tv_sec = PERIOD_A;
    timerspec.it_interval.tv_nsec = 0;
    timerspec.it_value.tv_sec = PERIOD_A;
    timerspec.it_value.tv_nsec = 0;

    timer_create(CLOCK_MONOTONIC, &sigev, &timer);

    while(1) {
        timer_settime(timer, 0, &timerspec, NULL);

        sigwait(&sigset, &signum);

        pthread_mutex_lock(&data->mutexA);

        data->contadorA += INCREMENT_A;

        printf("[A] Incrementing counter A: %d.\n", data->contadorA);

        if(data->contadorA % MULTIPLE_A == 0) {
            kill(getpid(), SIGNAL_A_CALL_C);
        }

        pthread_mutex_unlock(&data->mutexA);
    }

    return NULL;
}

void* TareaB(void* args) {
    //Get data
    struct Data* data = args;

    //Init signal stuff
    sigset_t sigset;
    struct sigevent sigev;
    int signum;

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGNAL_TIMER_B);

    sigev.sigev_notify = SIGEV_SIGNAL;
    sigev.sigev_signo = SIGNAL_TIMER_B;

    //Init timer
    timer_t timer;
    struct itimerspec timerspec;

    timerspec.it_interval.tv_sec = PERIOD_B;
    timerspec.it_interval.tv_nsec = 0;
    timerspec.it_value.tv_sec = PERIOD_B;
    timerspec.it_value.tv_nsec = 0;

    timer_create(CLOCK_MONOTONIC, &sigev, &timer);

    while(1) {
        timer_settime(timer, 0, &timerspec, NULL);

        sigwait(&sigset, &signum);

        pthread_mutex_lock(&data->mutexB);

        data->contadorB += INCREMENT_B;

        printf("[B] Incrementing counter B: %d.\n", data->contadorB);

        if(data->contadorB % MULTIPLE_B == 0) {
            kill(getpid(), SIGNAL_B_CALL_C);
        }

        pthread_mutex_unlock(&data->mutexB);
    }
}

void* TareaC(void* args) {
    //Get data
    struct Data* data = args;

    //Init signal
    sigset_t sigset;
    int signum;

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGNAL_A_CALL_C);
    sigaddset(&sigset, SIGNAL_B_CALL_C);

    while(1) {
        sigwait(&sigset, &signum);

        pthread_mutex_lock(&data->mutexA);
        pthread_mutex_lock(&data->mutexB);

        if(signum == SIGNAL_A_CALL_C) {
            printf("[C] - Counter A is divisible by 10.\n");
        }

        if(signum == SIGNAL_B_CALL_C) {
            printf("[C] - Counter B is divisible by 5.\n");
        }

        pthread_mutex_unlock(&data->mutexA);
        pthread_mutex_unlock(&data->mutexB);
    }
}

int main() {
    if(mlockall(MCL_CURRENT | MCL_FUTURE)) {
        return -1;
    }

    //Init signal stuff
    sigset_t sigset;

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGNAL_TIMER_A);
    sigaddset(&sigset, SIGNAL_TIMER_B);
    sigaddset(&sigset, SIGNAL_A_CALL_C);
    sigaddset(&sigset, SIGNAL_B_CALL_C);

    pthread_sigmask(SIG_BLOCK, &sigset, NULL);

    //Init data and mutexes
    struct Data data;
    data.contadorA = 0;
    data.contadorB = 0;
    pthread_mutex_init(&data.mutexA, NULL);
    pthread_mutex_init(&data.mutexB, NULL);

    //Init threads
    pthread_t threadA, threadB, threadC;
    pthread_create(&threadA, NULL, TareaA, &data);
    pthread_create(&threadB, NULL, TareaB, &data);
    pthread_create(&threadC, NULL, TareaC, &data);

    //Run threads
    pthread_join(threadA, NULL);
    pthread_join(threadB, NULL);
    pthread_join(threadC, NULL);

    pthread_mutex_destroy(&data.mutexA);
    pthread_mutex_destroy(&data.mutexB);

    return 0;
}