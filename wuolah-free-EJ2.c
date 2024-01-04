#include <sys/mman.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sched.h>
#include <math.h>

#define PER_A 1
#define PER_B 4
#define PER_C 4

#define PRIO_A 10
#define PRIO_B 9
#define PRIO_C 8

#define N 20
#define M 25

#define PrecioEnergia 10.0
#define Confort 22.0



struct Data{
	float paramEnerg;
	int aire_cale; // 0 = aire / 1 = calefaccion
	float gasto;
	pthread_mutex_t mutex;
};

void error(char *msg){
	printf("EROR: %s \n", msg);
	exit(-1);
}

void *TareaA(void *arg){
	struct Data *data = arg;
	float tempAct;
	struct timespec periodo;
	struct timespec next;

	periodo.tv_sec = PER_A;
	periodo.tv_nsec = 0;

	clock_gettime(CLOCK_MONOTONIC, &next);

	while(1){
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, 0);

		pthread_mutex_lock(&data->mutex);

		tempAct = rand() % (M+1-N) + N; // ENTRE N Y M
		printf("TEMPERATURA RANDOM: %f \n", tempAct);
		data->paramEnerg = (Confort-tempAct) * 2.5;

		pthread_mutex_unlock(&data->mutex);

		next.tv_sec += periodo.tv_sec;
		next.tv_nsec += periodo.tv_nsec;

		next.tv_sec += next.tv_nsec / 1000000000;
		next.tv_nsec = next.tv_nsec % 1000000000;
	}

	return NULL;
}

void *TareaB(void *arg){
	struct Data *data = arg;
	struct timespec periodo;
	struct timespec next;

	periodo.tv_sec = PER_B;
	periodo.tv_nsec = 0;

	clock_gettime(CLOCK_MONOTONIC, &next);

	while(1){
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, 0);

		pthread_mutex_lock(&data->mutex);

		if(data->paramEnerg < 0.0){
			data->aire_cale = 0;
		} else {
			data->aire_cale = 1;
		}

		data->gasto = fabs(data->paramEnerg) * PrecioEnergia;

		pthread_mutex_unlock(&data->mutex);

		next.tv_sec += periodo.tv_sec;
		next.tv_nsec += periodo.tv_nsec;

		next.tv_sec += next.tv_nsec / 1000000000;
		next.tv_nsec = next.tv_nsec % 1000000000;
	}

	return NULL;
}

void *TareaC(void *arg){
	struct Data *data = arg;
	struct timespec periodo;
	struct timespec next;

	periodo.tv_sec = PER_C;
	periodo.tv_nsec = 0;

	clock_gettime(CLOCK_MONOTONIC, &next);

	while(1){
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, 0);

		pthread_mutex_lock(&data->mutex);

		printf("CONTROL - PARAMETRO ENERGETICO: %f \n", data->paramEnerg);
		printf("CONTROL - ESTADO AIRE: %s \n", data->aire_cale == 0 ? "aire":"calefaccion");
		printf("CONTROL - GASTO ULTIMA ACCION: %f \n", data->gasto);

		pthread_mutex_unlock(&data->mutex);

		next.tv_sec += periodo.tv_sec;
		next.tv_nsec += periodo.tv_nsec;

		next.tv_sec += next.tv_nsec / 1000000000;
		next.tv_nsec = next.tv_nsec % 1000000000;
	}

	return NULL;
}

int main(){
	pthread_t tA, tB, tC;
	struct sched_param param;
	pthread_attr_t attr;
	int policy;
	struct Data data;

	if(mlockall(MCL_CURRENT | MCL_FUTURE)){
		error("MLOCKALL");
	}

	data.paramEnerg = 0.0;
	data.gasto = 0.0;
	srand(time(NULL));

	pthread_mutex_init(&data.mutex, NULL);

	pthread_attr_init(&attr);
	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&attr, SCHED_OTHER);

	param.sched_priority = PRIO_A;
	pthread_attr_setschedparam(&attr, &param);
	pthread_create(&tA, &attr, TareaA, &data);
	param.sched_priority = PRIO_B;
	pthread_attr_setschedparam(&attr, &param);
	pthread_create(&tB, &attr, TareaB, &data);
	param.sched_priority = PRIO_C;
	pthread_attr_setschedparam(&attr, &param);
	pthread_create(&tC, &attr, TareaC, &data);

	pthread_join(tA, NULL);
	pthread_join(tB, NULL);
	pthread_join(tC, NULL);

	pthread_attr_destroy(&attr);

	pthread_mutex_destroy(&data.mutex);

	return 0;
}
