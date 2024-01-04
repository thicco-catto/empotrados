#include <sys/mman.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define PERIODO_A 1
#define PERIODO_B 2
#define INCREMENTO_A 1
#define INCREMENTO_B 2

struct Data {
    int contadorA;
    int contadorB;
    pthread_mutex_t mutexA;
	pthread_mutex_t mutexB;
};

void *TareaA(void* arg) {
    struct Data *data = arg;
    sigset_t sigset;
    struct sigevent sgev;
	struct itimerspec its;
	timer_t timer;
	int signum;

    sgev.sigev_notify = SIGEV_SIGNAL;
    sgev.signo = SIGRTMIN + 1;

    its.it_interval.tv_sec = PERIODO_A;
	its.it_interval.tv_nsec = 0;
	its.it_value.tv_sec = PERIODO_A;
	its.it_value.tv_nsec = 0;

    sigemptyset(&sigset);
	sigaddset(&sigset, SIGRTMIN+1); // TIMER A

    timer_create(CLOCK_MONOTONIC, &sgev, &timer);

    while(1) {
        timer_settime(timer, 0, &its, NULL);

        sigwait(&sigset, &signum);

        pthread_mutex_lock(data->mutexA);

		data->contadorA += INCREMENTO_A;
		printf("TAREA A: CONTADOR = %d \n", data->contadorA);

		if(contadorA % 10 == 0){
			kill(getpid(), SIGRTMIN+3);
		}

		pthread_mutex_unlock(data->mutexA);
    }

    return NULL;
}

void *TareaB(void* arg) {
    struct Data* data = arg;

    sigset_t sigset;
    struct sigevent sigev;
    int signum;

    sigev.sigev_notify = SIGEV_SIGNAL;
    sigev.signo = SIGRTMIN+2;

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGRTMIN+2);

    timer_t timer;
    struct itimerspec timerspec;

    timerspec.it_interval.tv_sec = PERIODO_B;
    timerspec.it_interval.tv_nsec = 0;
    timerspec.it_value.tv_sec = PERIODO_B;
    timerspec.it_value.tv_nsec = 0;

    timer_create(CLOCK_MONOTONIC, &sigev, &timer)

    while(1) {
        timer_settime(timer, 0, &timerspec, NULL);

        sigwait(&sigset, &signum);

        pthread_mutex_lock(&(data->mutexB));

        data->contadorB += INCREMENTO_B;
        printf("TAREA B: CONTADOR = %d \n", data->contadorB);

        if(data->contadorB % 5) {
            kill(getpid(), SIGRTMIN+4);
        }

        pthread_mutex_unlock(&(data->mutexB));
    }

    return NULL;
}

void *TareaC(void* arg) {
    struct Data *data = arg;

    sigset_t sigset;
    int signum;

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGRTMIN+3);
    sigaddset(&sigset, SIGRTMIN+4);

    while(1) {
        sigwait(&sigset, &signum);

        pthread_mutex_lock(&data->mutexA);
        pthread_mutex_lock(&data->mutexB);

        if(signum == 3) {
            printf("TAREA C: El contador A es multiplo de 10.\n");
        }

        if(signum == 4) {
            printf("TAREA C: El contador B es multiplo de 5.\n");
        }

        pthread_mutex_unlock(&data->mutexA);
        pthread_mutex_unlock(&data->mutexB);
    }
}

int main() {
    struct Data data;
    sigset_t sigset;
    pthread_t threadA, threadB, threadC;

    if(mlockall(MCL_CURRENT | MCL_FUTURE)) {
        return -1;
    }

    data.contadorA = 0;
    data.contadorB = 0;
    pthread_mutex_init(&data.mutexA, NULL);
    pthread_mutex_init(&data.mutexB, NULL);

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGRTMIN+1); // TIMER A
	sigaddset(&sigset, SIGRTMIN+2); // TIMER B
	sigaddset(&sigset, SIGRTMIN+3); // A -> C
	sigaddset(&sigset, SIGRTMIN+4); // B -> C

    pthread_sigmask(SIG_BLOCK, &sigset, NULL);

    pthread_create(&threadA, NULL, TareaA, &data);
    pthread_create(&threadB, NULL, TareaB, &data);
    pthread_create(&threadC, NULL, TareaC, &data);

    pthread_join(threadA, NULL);
    pthread_join(threadB, NULL);
    pthread_join(threadC, NULL);

    pthread_mutex_destroy(&data.mutexA);
    pthread_mutex_destroy(&data.mutexB);

    return 0;
}