/*
    pstress.c
    Copyright (C) 2010 Dalmo Cirne

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/time.h>

#define outFileName "outfiles%d/gpl.%d.txt"
#define dirName "outfiles%d"
#define SCREENING 1

// Time tracker structure. Holds all time measurements relevant to this experiment.
typedef struct {
	double startTime, elapsedTime;
	double iteractionStartTime, iteractionElapsedTime;
	double counterStartTime, counterElapsedTime;
	double equationStartTime, equationElapsedTime;
	double fileStartTime, fileElapsedTime;
} TimeTracker;

// Structure containing the equation of 2 variables coordinates (x, y), its result (z) and the number of points to calculate
typedef struct {
	double x, y, z;
	int qtdPointsToCalculate;
} EquationCoordinate;

EquationCoordinate cord;
TimeTracker timeTracker;

// Global variables
const int numberIteractions = 100;
const int numberOfOutputFiles = 100;
const int numberOfEquationPoints = 200000;
const unsigned long numberOfCounterIncrements = 100000000;
char *const inFileName = "gpl.txt";
unsigned long counter;
int dirNumber;
int equationCalculated = 0;
char *const FileName = "Posix.Stress.csv";

// Posix mutexes and semaphores
pthread_mutex_t mtxCounter;
pthread_mutex_t mtxCondition = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condEquation = PTHREAD_COND_INITIALIZER;

// Prototypes of functions executed by threads
void *incrementCounter(void *numIncs);
void *replicateFile(void *directoryNumber);
void *consumeEquationResults();
void *produceEquationResults();

// Prototypes of functions using or used by the threads
double getTimeMilliseconds();
void getEquationResult();
void calculateEquation(int i);

/* Executes the experiment 'numberIteractions' times. On each cycle integer, floating point, and I/O operations
 * are performed.
 */
int main(int argc, char *argv[]) {
	// Starts counting time
	timeTracker.startTime = getTimeMilliseconds();
	
	// Declaration of variables
	int numberOfThreads = 103; // 100 - counter; 2 - equation; 1 - file
	pthread_t threads[numberOfThreads];
	pthread_attr_t attr;
	unsigned long incrementsPerThread;
	char *dName;
	FILE *fp;
	double currentTime;
			
	incrementsPerThread = numberOfCounterIncrements / 100;
	cord.qtdPointsToCalculate = numberOfEquationPoints;
	
	// Creates file to host the experiment's log	
	fp = fopen(FileName, "w");
	fprintf(fp, "Elapsed Time, Iteration Time, Counter Time, Equation Time, File Time\n");

	// Initializes mutexes (some are already initialized), and thread attributes
	pthread_mutex_init(&mtxCounter, NULL);
	pthread_attr_init(&attr); 
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);	
	
	// Loops "numberIteractions" times to generate enough statistical data for analysis
	int iteraction, i, j;
	for (iteraction = 0; iteraction < numberIteractions; iteraction++) {
		timeTracker.iteractionStartTime = getTimeMilliseconds();
		counter = 0;
		timeTracker.counterStartTime = 0;
		
		cord.x = 0;
		cord.y = cord.x;

		dName = malloc(sizeof(dirName) + 4);
		sprintf((char *)dName, dirName, iteraction);
		mkdir((char *)dName, S_IRWXU | S_IRGRP | S_IROTH);
		free(dName);
		dName = NULL;
		dirNumber = iteraction;
		
        // Creates all the threads
		pthread_create(&threads[0], &attr, consumeEquationResults, NULL);
		pthread_create(&threads[1], &attr, produceEquationResults, NULL);
		pthread_create(&threads[2], &attr, replicateFile, (void *)&dirNumber);
		
		for (i = 3; i < numberOfThreads; i++) {
			pthread_create(&threads[i], &attr, incrementCounter, (void *)&incrementsPerThread);
		}
		
        // Waits for all threads to complete
		for (j = 0; j < numberOfThreads; j++) {
			pthread_join(threads[j], NULL);
		}
		
        // Saves result of the current iteration on the log file
		currentTime = getTimeMilliseconds();
		timeTracker.iteractionElapsedTime = currentTime - timeTracker.iteractionStartTime;
		timeTracker.elapsedTime = currentTime - timeTracker.startTime;
		fprintf(fp, "%.3f, %.3f, %.3f, %.3f, %.3f\n", timeTracker.elapsedTime, timeTracker.iteractionElapsedTime, timeTracker.counterElapsedTime,
				timeTracker.equationElapsedTime, timeTracker.fileElapsedTime);
		
        // If running on screening mode, also show the results on the screen
		#if SCREENING == 1 
			printf("%d -> %.3f, %.3f, %.3f, %.3f, %.3f\n", iteraction, timeTracker.elapsedTime, timeTracker.iteractionElapsedTime,
                   timeTracker.counterElapsedTime, timeTracker.equationElapsedTime, timeTracker.fileElapsedTime);
		#endif
	}
	
	fclose(fp); // Closes experiment's log file

    // Frees mutexes and thread attributes
    pthread_mutex_destroy(&mtxCounter);
    pthread_cond_destroy(&condEquation);
    pthread_mutex_destroy(&mtxCondition);
	pthread_attr_destroy(&attr);
	pthread_exit(NULL);
}

// Increments a counter "numIncs" times. Only one thread can execute it at any given time.
void *incrementCounter(void *numIncs) {
	pthread_mutex_lock(&mtxCounter);

	if (timeTracker.counterStartTime == 0) {
		timeTracker.counterStartTime = getTimeMilliseconds(); 
	} 

    unsigned long incrementsPerThread;
	incrementsPerThread = *((unsigned long *)numIncs);
	
	int increment = 37, decrement = 36;
	unsigned long i = 0, temp;
    do {
		temp = counter;
		temp = temp + increment;
		temp = temp - decrement;
		counter = temp;
        i = i + 1;
    } while (i < incrementsPerThread);
	
	timeTracker.counterElapsedTime = getTimeMilliseconds() - timeTracker.counterStartTime;
	
	pthread_mutex_unlock(&mtxCounter);
	pthread_exit(NULL);
}

// Reads a file and replicates its contents "numberOfOutputFiles" times inside a directory 
void *replicateFile(void *directoryNumber) {
	timeTracker.fileStartTime = getTimeMilliseconds(); 
	
	char *outFile;
	FILE *inFileHandle, *outFileHandle;
    size_t readSize, fileSize;
	int dirNumber;

    dirNumber = *((int *)directoryNumber);

    inFileHandle = fopen(inFileName, "r");
    fseek(inFileHandle, 0, SEEK_END);
    fileSize = ftell(inFileHandle);
    rewind(inFileHandle);

    char *buffer = (char *)malloc(fileSize);

	int i;
	for (i = 0; i < numberOfOutputFiles; i++) {
		outFile = (char *)malloc(sizeof(outFileName) + 20);
		sprintf((char *)outFile, outFileName, dirNumber, i);
		outFileHandle = fopen(outFile, "w");
		while (1) {
            readSize = fread(buffer, 1, fileSize, inFileHandle);

			if (readSize > 0) {
				fwrite(buffer, 1, fileSize, outFileHandle);
			} else {
				break;
			}
		}

		fclose(outFileHandle);
		free(outFile);
		outFile = NULL;
        rewind(inFileHandle);
	}

	fclose(inFileHandle);
    free(buffer);
	
	timeTracker.fileElapsedTime = getTimeMilliseconds() - timeTracker.fileStartTime;
	
	pthread_exit(NULL);
}

// Consumes the result of the calculation of the equation. 
void *consumeEquationResults() {
	timeTracker.equationStartTime = getTimeMilliseconds(); 
	
	int i;
	for (i = 0; i < cord.qtdPointsToCalculate; i++) {
		getEquationResult();
	}	
	
	timeTracker.equationElapsedTime = getTimeMilliseconds() - timeTracker.equationStartTime;
	
	pthread_exit(NULL);
}

// Produces/calculates the results of the equation
void *produceEquationResults() {
	int i;
	
	for (i = 0; i < cord.qtdPointsToCalculate; i++) {
		calculateEquation(i);
	}

	pthread_exit(NULL);
}

/* Function called from inside the equation consumer thread. If the result is not ready to be consumed, than
 * waits until receives a notification that the calculation is ready.
 */
void getEquationResult() {
	pthread_mutex_lock(&mtxCondition);

	while (equationCalculated != 1) {
		pthread_cond_wait(&condEquation, &mtxCondition);		
	}
	
    double x, y, z;
    
    x = cord.x;
    y = cord.y;
    z = cord.z;

    equationCalculated = 0;
	
	pthread_cond_signal(&condEquation);
	pthread_mutex_unlock(&mtxCondition);	
}

/* Function called from inside the equation producer thread. If the equation is calculated and the result is not
 * ready to be consumed, than waits until receives a notification from the consumer, before proceding to the 
 * next calculation.
 */
void calculateEquation(int i) {
	pthread_mutex_lock(&mtxCondition);
	while (equationCalculated == 1) {
		pthread_cond_wait(&condEquation, &mtxCondition);		
	}
	
	cord.z = exp(cos(sqrt(pow(cord.x, 2) + pow(cord.y, 2))));
	cord.x += i / 1.1;
	cord.y += i * 1.1;

	equationCalculated = 1;
	pthread_cond_signal(&condEquation);
	pthread_mutex_unlock(&mtxCondition);	
}

double getTimeMilliseconds() {
	struct timeval tv;
	struct timezone tz;

    gettimeofday(&tv, &tz);
    double milliseconds = tv.tv_sec * 1000 + (double)tv.tv_usec / 1000.0f;

//    printf("seconds: %ld, microseconds: %d, milliseconds: %.3f\n", tv.tv_sec, tv.tv_usec, milliseconds);

    return milliseconds;
}
