#ifndef AFLIC_H
#define AFLIC_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#define MAX_PLANES 190
#define MAX_RUNWAYS 5
#define SLEEP_CALLS 4
#define MAX_SLEEP 50
#define MIN_SLEEP 0
#define COLLISION_INTERVAL_CHECK 55

struct planeStruct {
	int number;
	int state;
	int isLanding;
	int numPlanes;
	int numRunways;
	int atRunway;
	int collisionPlane;
};

struct planeStruct plane[MAX_PLANES];

typedef enum {
	COLLISION_EVENT,
	TRAVELLING_TO_AIRPORT,
	ABOVE_AIRPORT,
	WAITING_FOR_DEPARTURE,
	STANDBY,
	AT_GATE,
	WANTS_RUNWAY,
	LANDED,
	AT_GATE_LANDING,
	TAKEOFF,
	GONE

} to;

struct runwayStruct {
	int isOpen;
};	

struct runwayStruct runway[MAX_RUNWAYS];

struct queueStruct {
int element;
};
struct queueStruct planeQueue[MAX_PLANES];



pthread_mutex_t lock;



int integerChecker(char*);
int otherMaxSleep(int);
void randomSleep(int,int);
void collisionEvent(int);
void collisionDetection(int,int);
void* simStart(void *);
void queuePop(int);
void queueInsert(int, int);

#endif
