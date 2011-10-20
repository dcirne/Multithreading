/*
    wthreads.cpp
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

#include "stdafx.h"
#include <math.h>
#include <windows.h>

#define outFileName "outfiles%d\\gpl.%d.txt"
#define dirName "outfiles%d"
#define SCREENING 1

// Structure containing the equation of 2 variables coordinates (x, y), its result (z) and the number of points to calculate
typedef struct {
	double x, y, z; 
	int qtdPointsToCalculate;
} EquationCoordinate;

// Time tracker structure. Holds all time measurements relevant to this experiment. 
typedef struct {
	DWORD startTime, elapsedTime;
	DWORD iteractionStartTime, iteractionElapsedTime;
	DWORD counterStartTime, counterElapsedTime;
	DWORD equationStartTime, equationElapsedTime;
	DWORD fileStartTime, fileElapsedTime;
} TimeTracker;

EquationCoordinate cord;
TimeTracker timeTracker;

// Global variables
unsigned long counter;
int dirNumber;
const int numberIteractions = 100;
const int numberOfOutputFiles = 100;
const int numberOfEquationPoints = 200000;
const unsigned long numberOfCounterIncrements = 100000000;
char *const FileName = "Windows.Threads.csv";
char *const inFileName = "gpl.txt";

// Prototypes of functions executed by threads
DWORD WINAPI incrementCounter(LPVOID numIncs);
DWORD WINAPI replicateFile(LPVOID directoryNumber);
DWORD WINAPI consumeEquationResults(LPVOID foo);

// Prototypes of functions using or used by the threads
void getEquationResult();
void calculateEquation(int i);
DWORD getTimeMilliseconds();

/* Executes the experiment 'numberIteractions' times. On each cycle integer, floating point, and I/O 
 * operations are performed.
 */
int _tmain(int argc, _TCHAR* argv[]) {
	// Starts counting time
	timeTracker.startTime = getTimeMilliseconds();
	
	// Declaration of variables
	const int numberOfThreads = 3; // 1 - counting; 1 - equation; 1 - file
	HANDLE threads[numberOfThreads];
	unsigned long incrementsPerThread;
	char *dName, *createDirCmd;
	FILE *fp;
	DWORD currentTime;
			
	incrementsPerThread = numberOfCounterIncrements;
	cord.qtdPointsToCalculate = numberOfEquationPoints;
	
	// Creates file to host the experiment's log	
	fopen_s(&fp, FileName, "w");
	fprintf(fp, "Elapsed Time, Iteration Time, Counter Time, Equation Time, File Time\n");

    // Loops "numberIteractions" times to generate enough statistical data for analysis
	int iteraction, j;
	for (iteraction = 0; iteraction < numberIteractions; iteraction++) {
		timeTracker.iteractionStartTime = getTimeMilliseconds();
		counter = 0;
		timeTracker.counterStartTime = 0;
		
		cord.x = 0;
		cord.y = cord.x;

		// Creates the directories to copy the file to.
		dName = (char *)malloc(sizeof(dirName) + 4);
		createDirCmd = (char *)malloc(sizeof(dirName) + 11);
		sprintf_s((char *)dName, sizeof(dirName) + 4, dirName, iteraction);
		sprintf_s((char *)createDirCmd, sizeof(dirName) + 11, "mkdir %s", dName);
		system(createDirCmd);
		free(dName);
		free(createDirCmd);
		dName = NULL;
		dirNumber = iteraction;
		
        // Creates all the threads
        threads[0] = CreateThread(NULL, 0, incrementCounter, &incrementsPerThread, 0, NULL);
        threads[1] = CreateThread(NULL, 0, replicateFile, &dirNumber, 0, NULL);
        threads[2] = CreateThread(NULL, 0, consumeEquationResults, NULL, 0, NULL);
		
        // Waits for all threads to complete
		for (j = 0; j < numberOfThreads; j++) {
            WaitForSingleObject(threads[j], INFINITE);
		}
		
        // Saves result of the current iteration on the log file
		currentTime = getTimeMilliseconds();
		timeTracker.iteractionElapsedTime = currentTime - timeTracker.iteractionStartTime;
		timeTracker.elapsedTime = currentTime - timeTracker.startTime;
		fprintf(fp, "%ld, %ld, %ld, %ld, %ld\n", timeTracker.elapsedTime, timeTracker.iteractionElapsedTime, timeTracker.counterElapsedTime, 
				timeTracker.equationElapsedTime, timeTracker.fileElapsedTime);
		
        // If running on screening mode, also show the results on the screen
		#if SCREENING == 1 
			printf("%d -> %ld, %ld, %ld, %ld, %ld\n", iteraction, timeTracker.elapsedTime, timeTracker.iteractionElapsedTime, 
                   timeTracker.counterElapsedTime, timeTracker.equationElapsedTime, timeTracker.fileElapsedTime);
		#endif
	}
	
	fclose(fp); // Closes experiment's log file
	return 0;
}

// Increments a counter "numIncs" times. Only one thread can execute it at any given time.
DWORD WINAPI incrementCounter(LPVOID numIncs) {
	timeTracker.counterStartTime = getTimeMilliseconds(); 

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

	return 0;
}

// Reads a file and replicates its contents "numberOfOutputFiles" times inside a directory 
DWORD WINAPI replicateFile(LPVOID directoryNumber) {
	timeTracker.fileStartTime = getTimeMilliseconds(); 
	
	char *outFile;
	FILE *inFileHandle, *outFileHandle;
	size_t readSize, fileSize;
	int dirNumber;

	dirNumber = *((int *)directoryNumber);

	fopen_s(&inFileHandle, inFileName, "r");
	fseek(inFileHandle, 0, SEEK_END);
	fileSize = ftell(inFileHandle);
	rewind(inFileHandle);

	char *buffer = (char *)malloc(fileSize);

	int i;
	for (i = 0; i < numberOfOutputFiles; i++) {
		outFile = (char *)malloc(sizeof(outFileName) + 20);
		sprintf_s((char *)outFile, sizeof(outFileName) + 7, outFileName, dirNumber, i);

		fopen_s(&outFileHandle, outFile, "w");
		while (true) {
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

	return 0;
}

// Consumes the result of the calculation of the equation. 
DWORD WINAPI consumeEquationResults(LPVOID foo) {
	timeTracker.equationStartTime = getTimeMilliseconds(); 
	
	int i;
	for (i = 0; i < cord.qtdPointsToCalculate; i++) {
		calculateEquation(i);
		getEquationResult();
	}	
	
	timeTracker.equationElapsedTime = getTimeMilliseconds() - timeTracker.equationStartTime;

	return 0;
}

/* Function called from inside the equation consumer thread. If the result is not ready to be consumed, than
 * waits until receives a notification that the calculation is ready.
 */
void getEquationResult() {
    double x, y, z;
    
    x = cord.x;
    y = cord.y;
    z = cord.z;
}

/* Function called from inside the equation producer thread. If the equation is calculated and the result is not
 * ready to be consumed, than waits until receives a notification from the consumer, before proceding to the 
 * next calculation.
 */
void calculateEquation(int i) {
	cord.z = exp(cos(sqrt(pow(cord.x, 2) + pow(cord.y, 2))));
	cord.x += i / 1.1;
	cord.y += i * 1.1;
}

DWORD getTimeMilliseconds() {
	DWORD milliseconds;
	milliseconds = GetTickCount();

	return milliseconds;
}
