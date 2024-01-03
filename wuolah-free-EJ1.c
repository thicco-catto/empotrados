#include <sys/mman.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define PER_A 1
#define PER_B 2
#define AV_A 1
#define AV_B 2

int contadorA = 0;
int contadorB = 0;

struct mutex{
	pthread_mutex_t mutexA;
	pthread_mutex_t mutexB;
};

void error(char *msg){
	printf("EROR: %s \n", msg);
	exit(-1);
}

void *TareaA(void* arg){
	pthread_mutex_t *mutex = arg;
	sigset_t sigset;
	struct sigevent sgev;
	struct itimerspec its;
	timer_t timer;
	int signum;

	sgev.sigev_notify = SIGEV_SIGNAL;
	sgev.sigev_signo = SIGRTMIN+1;

	its.it_interval.tv_sec = PER_A;
	its.it_interval.tv_nsec = 0;
	its.it_value.tv_sec = PER_A;
	its.it_value.tv_nsec = 0;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGRTMIN+1); // TIMER A

	timer_create(CLOCK_MONOTONIC, &sgev, &timer);
	while(1){
		timer_settime(timer, 0, &its, NULL);

		sigwait(&sigset, &signum);

		pthread_mutex_lock(mutex);

		contadorA += AV_A;
		printf("TAREA A: CONTADOR = %d \n", contadorA);

		if(contadorA % 10 == 0){
			kill(getpid(), SIGRTMIN+3);
		}

		pthread_mutex_unlock(mutex);

	}

	return NULL;
}

void *TareaB(void* arg){
	pthread_mutex_t *mutex = arg;
	sigset_t sigset;
	struct sigevent sgev;
	struct itimerspec its;
	timer_t timer;
	int signum;

	sgev.sigev_notify = SIGEV_SIGNAL;
	sgev.sigev_signo = SIGRTMIN+2;

	its.it_interval.tv_sec = PER_B;
	its.it_interval.tv_nsec = 0;
	its.it_value.tv_sec = PER_B;
	its.it_value.tv_nsec = 0;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGRTMIN+2); // TIMER B

	timer_create(CLOCK_MONOTONIC, &sgev, &timer);
	while(1){
		timer_settime(timer, 0, &its, NULL);

		sigwait(&sigset, &signum);

		pthread_mutex_lock(mutex);

		contadorB += AV_B;
		printf("TAREA B: CONTADOR = %d \n", contadorB);

		if(contadorB % 5 == 0){
			kill(getpid(), SIGRTMIN+4);
		}

		pthread_mutex_unlock(mutex);

	}

	return NULL;

}

void *TareaC(void* arg){
	struct mutex *mutex = arg;
	sigset_t sigset;
	int signum;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGRTMIN+3); // A -> C
	sigaddset(&sigset, SIGRTMIN+4); // B -> C

	while(1){
		sigwait(&sigset, &signum);

		pthread_mutex_lock(&mutex->mutexA);
		pthread_mutex_lock(&mutex->mutexB);

		if(signum == SIGRTMIN+3){
			printf("C - EL CONTADOR A ES MULTIPLO DE 10. \n");
		}

		if(signum == SIGRTMIN+4){
			printf("C - EL CONTADOR B ES MULTIPLO DE 5. \n");
		}

		pthread_mutex_unlock(&mutex->mutexA);
		pthread_mutex_unlock(&mutex->mutexB);

	}

	return NULL;

}

int main(){
	struct mutex mutexAB;
	sigset_t sigset;
	pthread_t tA, tB, tC;

	if(mlockall(MCL_CURRENT | MCL_FUTURE)){
		error("MLOCKALL");
	}

	pthread_mutex_init(&mutexAB.mutexA, NULL);
	pthread_mutex_init(&mutexAB.mutexB, NULL);

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGRTMIN+1); // TIMER A
	sigaddset(&sigset, SIGRTMIN+2); // TIMER B
	sigaddset(&sigset, SIGRTMIN+3); // A -> C
	sigaddset(&sigset, SIGRTMIN+4); // B -> C

	pthread_sigmask(SIG_BLOCK, &sigset, NULL);

	pthread_create(&tA, NULL, TareaA, &mutexAB.mutexA);
	pthread_create(&tB, NULL, TareaB, &mutexAB.mutexB);
	pthread_create(&tC, NULL, TareaC, &mutexAB);

	pthread_join(tA, NULL);
	pthread_join(tB, NULL);
	pthread_join(tC, NULL);

	pthread_mutex_destroy(&mutexAB.mutexA);
	pthread_mutex_destroy(&mutexAB.mutexB);

	return 0;
}