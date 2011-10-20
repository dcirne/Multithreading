/*
    plinear.c
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
char *const FileName = "Posix.Linear.csv";

// Prototypes of functions 
double getTimeMilliseconds();
void incrementCounter(unsigned long numIncs);
void replicateFile(int directoryNumber);
void ProduceEquationResults(int i);
void ConsumeEquationResults();

/* Executes the experiment 'numberIteractions' times. On each cycle integer, floating point, and I/O operations
 * are performed.
 */
int main(int argc, char *argv[]) {
	// Starts counting time
	timeTracker.startTime = getTimeMilliseconds();
	
	// Declaration of variables
	char *dName;
	FILE *fp;
	double currentTime;
			
	cord.qtdPointsToCalculate = numberOfEquationPoints;
	
	// Creates file to host the experiment's log	
	fp = fopen(FileName, "w");
	fprintf(fp, "Elapsed Time, Iteration Time, Counter Time, Equation Time, File Time\n");

	// Loops "numberIteractions" times to generate enough statistical data for analysis
	int iteraction, i;
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
		
        // Executes the tasks in linear (serial) fashion.
        
        // Increments the counter
        incrementCounter(numberOfCounterIncrements);
        
        // Calculates equation coordinates
        timeTracker.equationStartTime = getTimeMilliseconds(); 
        for (i = 0; i < cord.qtdPointsToCalculate; i++) {
            ProduceEquationResults(i);
            ConsumeEquationResults();
        }
        timeTracker.equationElapsedTime = getTimeMilliseconds() - timeTracker.equationStartTime;

        // Replicates file
        replicateFile(dirNumber);

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
    return 0;
}

// Increments a counter "numIncs" times. Only one thread can execute it at any given time.
void incrementCounter(unsigned long numIncs) {
	timeTracker.counterStartTime = getTimeMilliseconds(); 

	int increment = 37, decrement = 36;
	unsigned long i = 0, temp;
    do {
		temp = counter;
		temp = temp + increment;
		temp = temp - decrement;
		counter = temp;
        i = i + 1;
    } while (i < numIncs);
	
	timeTracker.counterElapsedTime = getTimeMilliseconds() - timeTracker.counterStartTime;
}

// Reads a file and replicates its contents "numberOfOutputFiles" times inside a directory 
void replicateFile(int directoryNumber) {
	timeTracker.fileStartTime = getTimeMilliseconds(); 
	
	char *outFile;
	FILE *inFileHandle, *outFileHandle;
    size_t readSize, fileSize;
	
    inFileHandle = fopen(inFileName, "r");
    fseek(inFileHandle, 0, SEEK_END);
    fileSize = ftell(inFileHandle);
    rewind(inFileHandle);

    char *buffer = (char *)malloc(fileSize);

    int i;
	for (i = 0; i < numberOfOutputFiles; i++) {
		outFile = (char *)malloc(sizeof(outFileName) + 20);
		sprintf((char *)outFile, outFileName, directoryNumber, i);
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
}

// Produces/calculates the results of the equation
void ProduceEquationResults(int i) {
    cord.z = exp(cos(sqrt(pow(cord.x, 2) + pow(cord.y, 2))));
	cord.x += i / 1.1;
	cord.y += i * 1.1;
}

// Consumes the result of the calculation of the equation. 
void ConsumeEquationResults() {
    double x, y, z;
    
    x = cord.x;
    y = cord.y;
    z = cord.z;
}

double getTimeMilliseconds() {
	struct timeval tv;
	struct timezone tz;

    gettimeofday(&tv, &tz);
    double milliseconds = tv.tv_sec * 1000 + (double)tv.tv_usec / 1000.0f;

//    printf("seconds: %ld, microseconds: %d, milliseconds: %.3f\n", tv.tv_sec, tv.tv_usec, milliseconds);

    return milliseconds;
}
