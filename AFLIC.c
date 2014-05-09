#include "AFLIC.h"

// Main
int main() {
	pthread_mutex_init(&lock, NULL);
	srand(time(NULL));
	int numRunway = -1, numLandPlane = -1,
		numTakeoffPlane =-1, totalPlanes = -1;
	char input[256];

	// Get user input on number of runways, landing planes, planes taking off
	// Error check to make sure they conform to max and min and are numbers
	while(numRunway < 1 || numRunway > MAX_RUNWAYS) {
		printf("\nInput # of runways (Max: %d, Min: 1): ", MAX_RUNWAYS);
		scanf("%s", &input);
		numRunway = integerCheck(input);
		if(numRunway < 1 || numRunway > MAX_RUNWAYS)
			printf("\nIncorrect input for runways\n");
	}
	while(totalPlanes < 1 || totalPlanes > MAX_PLANES) {
		printf("\nMax # of planes combined is %d\n", MAX_PLANES);
		printf("\nInput # of planes landing: ");
		scanf("%s", &input);
		numLandPlane = integerCheck(input);
		printf("\nInput # of planes taking off: ");
		scanf("%s", &input);
		numTakeoffPlane = integerCheck(input);
		if(numLandPlane == -1 || numTakeoffPlane == -1)
			totalPlanes = -1;
		else
			totalPlanes = numLandPlane + numTakeoffPlane;
		if(totalPlanes < 1 || totalPlanes > MAX_PLANES)
			printf("\nIncorrect input for planes\n");
	}
	printf("\n");
	// Initializing the structures and
	// Creating threads to simulate each plane, plus collision detector
	pthread_t threads[totalPlanes];
	int *taskids[totalPlanes];
	int i, rc;

	for(i = 0; i < numRunway; i++) {
		runway[i].isOpen = 1;
	}

	for(i = 0; i < totalPlanes+1; i++) {
		plane[i].number = i+1;
		plane[i].numPlanes = totalPlanes;
		plane[i].numRunways = numRunway;
		plane[i].atRunway = -1;
		plane[i].collisionPlane = -1;
		if(i < numLandPlane) {
			planeQueue[i].element = -1;
			plane[i].state = TRAVELLING_TO_AIRPORT;
			plane[i].isLanding = 1;
		}
		else if(i < totalPlanes) {
			planeQueue[i].element = -1;
			plane[i].state = WAITING_FOR_DEPARTURE;
			plane[i].isLanding = 0;
		}
		else 
			plane[i].state = COLLISION_EVENT;

		rc = pthread_create(&threads[i], NULL, simStart, (void *) &plane[i]);

		if(rc) {
			printf("ERROR\n");
			exit(-1);
		} 
	}
	pthread_exit(NULL);	

	pthread_mutex_destroy(&lock);

	return 0;
}

// Return the integer if str is all integers, else return -1
int integerCheck(char *str) {
	int i;
	for(i = 0; i < strlen(str); i++) {
		if(str[i] < 48 || str[i] > 57)
			return -1;
	}
	return atoi(str);
}

// State and transitions of the airplanes and main output of the AFLIC system
void *simStart(void *threadarg) {
	int atRunway, plane1, plane2, i;
	struct planeStruct *my_data;
	my_data= (struct planeStruct*) threadarg;
	int taskid = my_data->number-1;
	int maxSleep = otherMaxSleep(my_data->numPlanes);
	// Keep going until the plane (thread) has reached its final state
	while(my_data->state != GONE) {
		pthread_mutex_t *locks;
		// Only sleep at random intervals when the threads aren't at these states
		if(my_data->state != WANTS_RUNWAY &&
				my_data->state != TRAVELLING_TO_AIRPORT &&
				my_data->state != WAITING_FOR_DEPARTURE) {
			randomSleep(MAX_SLEEP, MIN_SLEEP);
		}
		atRunway = my_data->atRunway;

		// Check the state of each plane and act accordingly
		switch(my_data->state) {
			case COLLISION_EVENT: // Only 1 thread has this state, decides collision
				collisionEvent(my_data->numPlanes);
				break;
				// Start of landing
			case TRAVELLING_TO_AIRPORT: // When airplane gets to airport, hover over.
				randomSleep(maxSleep, 0);
				my_data->state = ABOVE_AIRPORT;
				break;
			case ABOVE_AIRPORT: // Request runway, check for possible collision
				printf("-Plane %d is circling the airport, they request a runway\n",
						taskid+1);
				collisionDetection(taskid, my_data->collisionPlane);
				my_data->state = WANTS_RUNWAY;
				break;
				// Start of takeoff
			case WAITING_FOR_DEPARTURE: // When it's the plane's departure time, standby
				randomSleep(maxSleep, 0); 
				my_data->state = STANDBY;
				break;
			case STANDBY: // Standby request gate access
				printf("Plane %d wishes to proceed to the gates\n", taskid+1);
				randomSleep(MAX_SLEEP, MIN_SLEEP);
				printf("Permission granted for plane %d to proceed to the gates\n",
						taskid+1);
				my_data->state = AT_GATE;
				break;
			case AT_GATE: // Plane has passengers, request runway for takeoff
				printf("-Plane %d wishes to proceed to the runway\n", taskid+1);
				my_data->state = WANTS_RUNWAY;
				break;
			case WANTS_RUNWAY: // Wants runway (take off or landing) detect collisions
				queueInsert(taskid, my_data->numPlanes);
				int isGood = 0;
				pthread_mutex_lock(&lock);
				while(!isGood) {
					if(planeQueue[0].element == taskid) {
						i = 0;			
						for(i = 0; i < my_data->numRunways; i++){
							if(runway[i].isOpen) {
								isGood = 1;
								queuePop(my_data->numPlanes);
								runway[i].isOpen = 0;
								collisionDetection(taskid, my_data->collisionPlane);
								my_data->atRunway = i;
								if(my_data->isLanding) 
									my_data->state = LANDED;
								else 
									my_data->state = TAKEOFF;
								break;
							}
						}
					} 
				}
				pthread_mutex_unlock(&lock);
				break;
			case LANDED: // Plane has landed, request gate, free runway
				printf("Plane %d is on runway %d\n", taskid+1, atRunway+1);
				printf("Plane %d wishes to proceed to the gates\n", taskid+1);
				randomSleep(MAX_SLEEP, MIN_SLEEP);
				printf("Permission granted for plane %d to proceed to the gates\n",
						taskid+1);
				randomSleep(MAX_SLEEP, MIN_SLEEP);
				my_data->state = AT_GATE_LANDING;
				runway[atRunway].isOpen = 1;
				printf("Runway #%d is free\n", atRunway+1); 				
				break;

			case AT_GATE_LANDING: // Passengers have left plane, stop tracking plane
				printf("Plane %d's passengers have all left the plane\n", taskid+1);
				my_data->state = GONE;
				break;
			case TAKEOFF: // Taking off, detect collisions, stop tracking if OOF
				printf("~Plane %d is on runway %d\n", taskid+1, atRunway+1);
				randomSleep(MAX_SLEEP, MIN_SLEEP);
				printf("Plane %d has taken off\n", taskid+1);
				collisionDetection(taskid, my_data->collisionPlane);
				randomSleep(MAX_SLEEP, MIN_SLEEP);
				printf("Plane %d is out of range of AFLIC\n", taskid+1);
				runway[atRunway].isOpen = 1;
				printf("Runway #%d is free\n", atRunway+1);
				my_data->state = GONE;
				break;
		}
	}
	pthread_exit(NULL);
}

// Random Sleep function used to decide delays based on a min and max time frame
void randomSleep(int maxSleep, int minSleep) {
	int ms = rand() % maxSleep + minSleep;
	usleep(ms*1000);
}

// Returns an integer used for a delay to decide when planes want 
//  to depart or land, it makes sure they are not all clumped together
int otherMaxSleep(int numPlanes) {
	int dif = MAX_SLEEP - MIN_SLEEP;
	int ret = dif/2*SLEEP_CALLS*numPlanes;
	return ret;
}

// Randomly decide what planes are able to crash, timed at a defined interval
void collisionEvent(int totalPlanes) {
	int i;
	int keepGoing = 1;
	while(keepGoing) {
		int plane1 = -1;
		int plane2 = -1;
		usleep(COLLISION_INTERVAL_CHECK * 1000);
		while(plane1 == plane2 ||
				(plane[plane1].state == GONE || plane[plane2].state == GONE)) {
			plane1 = rand() % totalPlanes;
			plane2 = rand() % totalPlanes;

			if(plane[plane1].state == GONE && plane[plane2].state == GONE) {
				keepGoing = 0;
				for(i = 0; i < totalPlanes; i++) {
					if(plane[i].state != GONE) {
						keepGoing = 1;
						break;
					} 
				}
				if(keepGoing == 0)
					break;
			}
		}   

		plane[plane1].collisionPlane = plane2;
	}
	plane[totalPlanes].state = GONE;
}

// Check for any collision and report the possibility 
// then reset the flag for collision
void collisionDetection(int plane1, int plane2) {
	if(plane2 != -1) {
		if((plane[plane1].state == ABOVE_AIRPORT ||
					(plane[plane1].state == WANTS_RUNWAY &&
					 plane[plane1].isLanding) ||
					plane[plane1].state == TAKEOFF) &&
				(plane[plane2].state == ABOVE_AIRPORT ||
				 (plane[plane2].state == WANTS_RUNWAY &&
				  plane[plane2].isLanding) ||
				 plane[plane2].state == TAKEOFF)) {
			printf("*Possible collision detected between plane %d and plane %d.\n",
					plane1+1, plane2+1);
			plane[plane1].collisionPlane = -1;
		}
	}
}

// shift elements, popping the first element off the queue
void queuePop(int totalPlanes) {
	int i;
	for(i=0;i<totalPlanes;i++) {
		if(planeQueue[i].element == -1)
			break;
		else if(i+1 < totalPlanes) {
			planeQueue[i].element = planeQueue[i+1].element;
		}
	}

}

// insert the id of the plane in the last empty spot of the queue
void queueInsert(int element, int totalPlanes) {
	int i;
	for(i=0;i<totalPlanes;i++) {
		if(planeQueue[i].element == -1) {
			planeQueue[i].element = element;
			break;
		}
	}
}
