#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#define ENERGY_PARAM_PERIOD 1
#define ENERGY_PARAM_PRIORITY 20

#define AC_PERIOD 4
#define AC_PRIORITY 15

#define MONITOR_PERIOD 4
#define MONITOR_PRIORITY 10

#define POLICY SCHED_OTHER
#define MIN_TEMP 20
#define MAX_TEMP 25
#define ENERGY_PARAM_CONSTANT 2.5
#define CONFORT_TEMP 22.0
#define ENERGY_COST 10.0

struct Data {
    float energy_param;
    int cold_activate; //0 heat, 1 cold
    float cost;
    pthread_mutex_t mutex;
};

void* taskA(void* arg) {
    struct Data* data = arg;

    float new_temp;
    struct timespec period;
    struct timespec next;

    period.tv_sec = ENERGY_PARAM_PERIOD;
    period.tv_nsec = 0;

    clock_gettime(CLOCK_MONOTONIC, &next);

    while(1) {
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, 0);

        pthread_mutex_lock(&data->mutex);

        new_temp = rand() % (MAX_TEMP+1 - MIN_TEMP) + MIN_TEMP;
        printf("[A] - La nueva temperatura es %f.\n", new_temp);

        data->energy_param = (CONFORT_TEMP - new_temp) * ENERGY_PARAM_CONSTANT;
        
        pthread_mutex_unlock(&data->mutex);

        next.tv_sec += period.tv_sec;
		next.tv_nsec += period.tv_nsec;

		next.tv_sec += next.tv_nsec / 1000000000;
		next.tv_nsec = next.tv_nsec % 1000000000;
    }
}

void* taskB(void* arg) {
    struct Data* data = arg;

    struct timespec period;
    struct timespec next;

    period.tv_sec = AC_PERIOD;
    period.tv_nsec = 0;

    clock_gettime(CLOCK_MONOTONIC, &next);

    while(1) {
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, 0);

        pthread_mutex_lock(&data->mutex);

        data->cold_activate = data->energy_param < 0;
        data->cost = fabs(data->energy_param) * ENERGY_COST;

        pthread_mutex_unlock(&data->mutex);

        next.tv_sec += period.tv_sec;
        next.tv_nsec += period.tv_nsec;

        next.tv_sec += next.tv_nsec / 1000000000;
        next.tv_nsec = next.tv_nsec % 1000000000;
    }
}

void* taskC(void* arg) {
    struct Data* data;

    struct timespec period;
    struct timespec next;

    period.tv_sec = MONITOR_PERIOD;
    period.tv_nsec = 0;

    clock_gettime(CLOCK_MONOTONIC, &next);

    while(1) {
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, 0);

        pthread_mutex_lock(&data->mutex);

        printf("[C] - Energy parameter is %f.", data->energy_param);
        if(data->cold_activate) {
            printf("[C] - Cold activated.");
        } else {
            printf("[C] - Heat activated.");
        }
        printf("[C] - Cost of last action is %f.", data->cost);

        pthread_mutex_unlock(&data->mutex);

        next.tv_sec += period.tv_sec;
        next.tv_nsec += period.tv_nsec;

        next.tv_sec += next.tv_nsec / 1000000000;
        next.tv_nsec = next.tv_nsec % 1000000000;
    }
}

int main() {
    if(mlockall(MCL_CURRENT | MCL_FUTURE)) {
        return -1;
    }

    //Init data
    struct Data data;
    data.cold_activate = 0.0;
    data.cost = 0.0;
    data.energy_param = 0.0;
    
    pthread_mutex_init(&data.mutex, NULL);

    srand(time(NULL));    

    //Init threads
    pthread_t threadA, threadB, threadC;
    pthread_attr_t attr;
    struct sched_param param;

    //Init thread attributes
    pthread_attr_init(&attr);

    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, POLICY);

    //Create threads
    param.sched_priority = ENERGY_PARAM_PRIORITY;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&threadA, &attr, taskA, &data);

    param.sched_priority = AC_PRIORITY;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&threadB, &attr, taskB, &data);

    param.sched_priority = MONITOR_PRIORITY;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&threadC, &attr, taskC, &data);

    //Run threads
    pthread_join(threadA, NULL);
    pthread_join(threadB, NULL);
    pthread_join(threadC, NULL);

    //Destroy stuff
    pthread_attr_destroy(&attr);
    pthread_mutex_destroy(&data.mutex);

    return 0;
}