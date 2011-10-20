C_OPTIONS=-Wall -O2 -funroll-loops -msse -msse2 -msse3 -mssse3 -msse4.1 -mfpmath=sse,387 -ffast-math -m128bit-long-double -lm

EXPERIMENT_DIRECTORY=Experiment

# For Linux use CC=gcc, for OpenSolaris you may have to inform the path to gcc (CC=/usr/local/bin/gcc)
# You should choose between the commented lines below depending on which operating system you are compiling for
CC=gcc
#CC=/usr/local/bin/gcc

JAVAC=javac
JAR=jar

all: plinear pthreads pstress jlinear jstress jthreads

plinear: plinear/plinear.c directories
	$(CC) $(C_OPTIONS) plinear/plinear.c -o binaries/plinear
	mkdir -p $(EXPERIMENT_DIRECTORY)/plinear
	cp binaries/plinear $(EXPERIMENT_DIRECTORY)/plinear
	cp gpl.txt $(EXPERIMENT_DIRECTORY)/plinear

pthreads: pthreads/pthreads.c directories
	$(CC) $(C_OPTIONS) pthreads/pthreads.c -o binaries/pthreads -lpthread
	mkdir -p $(EXPERIMENT_DIRECTORY)/pthreads
	cp binaries/pthreads $(EXPERIMENT_DIRECTORY)/pthreads
	cp gpl.txt $(EXPERIMENT_DIRECTORY)/pthreads

pstress: pstress/pstress.c directories
	$(CC) $(C_OPTIONS) pstress/pstress.c -o binaries/pstress -lpthread
	mkdir -p $(EXPERIMENT_DIRECTORY)/pstress
	cp binaries/pstress $(EXPERIMENT_DIRECTORY)/pstress
	cp gpl.txt $(EXPERIMENT_DIRECTORY)/pstress

jlinear: jLinear/Linear.java directories
	$(JAVAC) jLinear/Linear.java
	$(JAR) cfm binaries/Linear.jar jLinear/META-INF/MANIFEST.MF jLinear/*.class
	rm -rf jLinear/*.class
	mkdir -p $(EXPERIMENT_DIRECTORY)/jLinear
	cp binaries/Linear.jar $(EXPERIMENT_DIRECTORY)/jLinear
	cp gpl.txt $(EXPERIMENT_DIRECTORY)/jLinear

jstress: jStress/Stress.java directories
	$(JAVAC) jStress/Stress.java
	$(JAR) cfm binaries/Stress.jar jStress/META-INF/MANIFEST.MF jStress/*.class
	rm -rf jStress/*.class
	mkdir -p $(EXPERIMENT_DIRECTORY)/jStress
	cp binaries/Stress.jar $(EXPERIMENT_DIRECTORY)/jStress
	cp gpl.txt $(EXPERIMENT_DIRECTORY)/jStress

jthreads: jThreads/Threads.java directories
	$(JAVAC) jThreads/Threads.java
	$(JAR) cfm binaries/Threads.jar jThreads/META-INF/MANIFEST.MF jThreads/*.class
	rm -rf jThreads/*.class
	mkdir -p $(EXPERIMENT_DIRECTORY)/jThreads
	cp binaries/Threads.jar $(EXPERIMENT_DIRECTORY)/jThreads
	cp gpl.txt $(EXPERIMENT_DIRECTORY)/jThreads

directories:
	mkdir -p binaries
	mkdir -p $(EXPERIMENT_DIRECTORY)
	
clean:
	rm -rf binaries
	rm -rf $(EXPERIMENT_DIRECTORY)
    