/*
    Stress.java
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

import java.io.*;
import java.util.ArrayList;
import java.util.List;
import java.util.Formatter;
import static java.lang.Math.exp;
import static java.lang.Math.cos;
import static java.lang.Math.sqrt;
import static java.lang.Math.pow;

public class Stress {
	private static final Integer numberIteractions = 100;
	private static final Integer numberOfOutputFiles = 100;
	private static final Integer numberOfEquationPoints = 200000;
    private static final Long numberOfCounterIncrements = 100000000L;
	private static final String dirName = "outfiles%d";
	protected static Long counter = new Long(0);
	private static String FileName;
	
	public static void main(String [] args) {
		// Starts counting time
		TimeTracker timeTracker = new TimeTracker();
		timeTracker.setStartTime();
		
    	// Declaration of variables
		Integer numberOfThreads = 103; // 100 - counting; 2 - equation; 1 - file
		Integer dirNumber;
		String dName = new String();
		Long incrementsPerThread;
        
        // If the file separator is a forward slash, then it is a Un*x based system. 
        // otherwise it is a Microsoft Windows
        if (("/").equals(System.getProperties().getProperty("file.separator"))) {
            FileName = "Java.Posix.Stress.csv";
        } else {
            FileName = "Java.Windows.Stress.csv";
        }
		
		// Creates file to host the experiment's log
		timeTracker.openTimeLog(FileName);

		incrementsPerThread = numberOfCounterIncrements / 100;
		
		// Loops "numberIteractions" times to generate enough statistical data for analysis
		for (int iteraction = 0; iteraction < numberIteractions; iteraction++) {
			timeTracker.setIteractionStartTime();
		    timeTracker.setCounterStartTime(0);
		    Stress.counter = 0L;

			List<Thread> thrds = new ArrayList<Thread>();
			
			dirNumber = iteraction;
		    dName = String.format(dirName, dirNumber); 
		    (new File(dName)).mkdir(); 
			
            // Creates all the threads
		 	EquationCalculator equationCalculator = new EquationCalculator(new EquationCoordinate(numberOfEquationPoints));
			ConsumeEquationResults consumeEquationResults = new ConsumeEquationResults(equationCalculator, timeTracker);
			ProduceEquationResult produceEquationResult = new ProduceEquationResult(equationCalculator);
			ReplicateFile replicateFile = new ReplicateFile(dirNumber, numberOfOutputFiles, timeTracker);
			
			thrds.add(new Thread(consumeEquationResults));
			thrds.add(new Thread(produceEquationResult));
			thrds.add(new Thread(replicateFile));
		    
			for (int i = 3; i < numberOfThreads; i++) {
				thrds.add( new Thread(new IncrementCounter(incrementsPerThread, timeTracker)) );
			}
			
			for (Thread t : thrds) {
				t.start();
			}
			
            // Waits for all threads to complete
			for (Thread t : thrds) {
			    try { t.join(); } catch (Exception ex) {}
			}
			
            // Saves result of the current iteration on the log file
			timeTracker.setIteractionAndElapsedTime();
			timeTracker.insertTimeLog();
			
            // If running on screening mode, also show the results on the screen
			if (Screen.SCREENING == true) {
				timeTracker.printLogEntry(iteraction);
			}
		}
		
		timeTracker.closeTimeLog();
	}
}

// Increments a counter "numIncs" times. Only one thread can execute it at any given time.
class IncrementCounter implements Runnable {
	private Long numIncs;
	private TimeTracker timeTracker;
	
	IncrementCounter(Long numIncs, TimeTracker timeTracker) {
		this.numIncs = numIncs;
		this.timeTracker = timeTracker;
	}
	
    @Override
	public synchronized void run() {
		if (timeTracker.getCounterStartTime() == 0) {
			timeTracker.setCounterStartTime();
		}
		
        int increment = 37, decrement = 36;
        Long i = 0L, temp;
        do {
            temp = Stress.counter;
            temp = temp + increment;
            temp = temp - decrement;
            Stress.counter = temp;
            i = i + 1;
        } while (i < numIncs);
		
		timeTracker.setCounterElapsedTime();
	}
}

// Reads a file and replicates its contents "numberOfOutputFiles" times inside a directory 
class ReplicateFile implements Runnable {
	private Integer directoryNumber;
	private Integer numberOfOutputFiles;
	private static final String inFileName = "gpl.txt";
	private static final String outFileName = "outfiles%d/gpl.%d.txt";
	private static final Integer fileBufferSize = 8192;
	private TimeTracker timeTracker;
	
	ReplicateFile(Integer directoryNumber, Integer numberOfOutputFiles, TimeTracker timeTracker) {
		this.directoryNumber = directoryNumber;
		this.numberOfOutputFiles = numberOfOutputFiles;
		this.timeTracker = timeTracker;
	}
	
    @Override
	public void run() {
		timeTracker.setFileStartTime();

		for (int i = 0; i < numberOfOutputFiles; i++) {
            String outFile = new String();
            String buffer; 

            File inputFile = new File(inFileName);

			outFile = String.format(outFileName, directoryNumber, i);
			File outputFile = new File(outFile);
			
			try {
				BufferedReader bufferedReader = new BufferedReader(new FileReader(inputFile), fileBufferSize);
				BufferedWriter bufferedWriter = new BufferedWriter(new FileWriter(outputFile), fileBufferSize);
			
				while ((buffer = bufferedReader.readLine()) != null) { 
					bufferedWriter.write(buffer); 
					bufferedWriter.newLine(); 
				} 
	
                bufferedWriter.flush();
				bufferedWriter.close();
                bufferedReader.close();
			} catch (IOException ex) {}
		}

		timeTracker.setFileElapsedTime();
	}
}

// Class containing the equation of 2 variables coordinates (x, y), its result (z) and the number of points to calculate
class EquationCoordinate {
	double x, y, z;
	int qtdPointsToCalculate;
	
	EquationCoordinate(int qtdPointsToCalculate) {
		x = 0;
		y = 0;
		z = 0;
		this.qtdPointsToCalculate = qtdPointsToCalculate;
	}
} 

// This class provides methods used by the producer and consumer of the equation results
class EquationCalculator {
	private boolean equationCalculated = false;
	protected EquationCoordinate equationCord;

	EquationCalculator(EquationCoordinate equationCord) {
		this.equationCord = equationCord;
	}
	
	/*
    * Method called from inside the equation consumer thread. If the result is not ready to be consumed, than
    * waits until receives a notification that the calculation is ready.
    */
    public synchronized EquationCoordinate getEquationResult() {
		while (!equationCalculated) {
			try {
				wait();
			} catch (Exception ex) {}
		}

		equationCalculated = false;
		notify();
		return equationCord;
	}
    
    /*
    * Method called from inside the equation producer thread. If the equation is calculated and the result is not
    * ready to be consumed, than waits until receives a notification from the consumer, before proceding to the 
    * next calculation.
    */
	public synchronized void calculateEquation(Integer i) {
		while (equationCalculated) {
			try {
				wait();
			} catch (Exception ex) {}
		}
		
		equationCord.z = exp(cos(sqrt(pow(equationCord.x, 2) + pow(equationCord.y, 2))));
		equationCord.x += i / 1.1;
		equationCord.y += i * 1.1;

		equationCalculated = true;
		notify();
	}
}

// Consume the result of the calculation of the equation.
class ConsumeEquationResults implements Runnable {
	EquationCalculator equationCalculator;
	private TimeTracker timeTracker;
	
	ConsumeEquationResults(EquationCalculator equationCalculator, TimeTracker timeTracker) {
		this.equationCalculator = equationCalculator;
		this.timeTracker = timeTracker;
	}
	
    @Override
	public void run() {
		timeTracker.setEquationStartTime();
        double x, y, z;
        EquationCoordinate eqResult;
		
		for (int i = 0; i < equationCalculator.equationCord.qtdPointsToCalculate; i++) {
			eqResult = equationCalculator.getEquationResult();

            x = eqResult.x;
            y = eqResult.y;
            z = eqResult.z;
		}	

		timeTracker.setEquationElapsedTime();
	}	
}

// Produces/calculates the results of the equation
class ProduceEquationResult implements Runnable {
	EquationCalculator equationCalculator;
	
	ProduceEquationResult(EquationCalculator equationCalculator) {
		this.equationCalculator = equationCalculator;
	}
	
    @Override
	public void run() {
		for (int i = 0; i < equationCalculator.equationCord.qtdPointsToCalculate; i++) {
			equationCalculator.calculateEquation(i);
		}
	}	
}

// Class used to keep track of time and record entries on the experiment's log file
class TimeTracker {
	private long startTime, elapsedTime;
	private long iteractionStartTime, iteractionElapsedTime;
	private long counterStartTime, counterElapsedTime;
	private long equationStartTime, equationElapsedTime;
	private long fileStartTime, fileElapsedTime;
	private BufferedWriter bufferedWriter;
	private StringBuffer results = new StringBuffer();

	TimeTracker() {
		counterStartTime = 0;
	}
	
	public void setStartTime() {
		this.startTime = System.currentTimeMillis();
	}
	
	public void setIteractionStartTime() {
		this.iteractionStartTime = System.currentTimeMillis();
	}
	
	public void setIteractionAndElapsedTime() {
		long currentTime = System.currentTimeMillis();
		this.iteractionElapsedTime = currentTime - this.iteractionStartTime;
		this.elapsedTime = currentTime - this.startTime;
	}	
	
	public long getCounterStartTime() {
		return this.counterStartTime;
	}
	
	public void setCounterStartTime(long ttTime) {
		this.counterStartTime = ttTime;
	}
	
	public void setCounterStartTime() {
		this.counterStartTime = System.currentTimeMillis();
	}
	
	public void setCounterElapsedTime() {
		this.counterElapsedTime = System.currentTimeMillis() - this.counterStartTime;
	}	
	
	public void setEquationStartTime() {
		this.equationStartTime = System.currentTimeMillis();
	}
	
	public void setEquationElapsedTime() {
		this.equationElapsedTime = System.currentTimeMillis() - this.equationStartTime;
	}	
	
	public void setFileStartTime() {
		this.fileStartTime = System.currentTimeMillis();
	}
	
	public void setFileElapsedTime() {
		this.fileElapsedTime = System.currentTimeMillis() - this.fileStartTime;
	}	
	
	public void openTimeLog(String fileName) {
		bufferedWriter = null;
		try {
			bufferedWriter = new BufferedWriter(new FileWriter(fileName));
			bufferedWriter.write("Elapsed Time, Iteration Time, Counter Time, Equation Time, File Time"); 
			bufferedWriter.newLine();
		} catch (IOException ex) {}
	}
	
	public void insertTimeLog() {
	    results.append( (new Formatter()).format("%10d", elapsedTime).toString() );
	    results.append(", ");

	    results.append( (new Formatter()).format("%10d", iteractionElapsedTime).toString() );
	    results.append(", ");

	    results.append( (new Formatter()).format("%10d", counterElapsedTime).toString() );
	    results.append(", ");

	    results.append( (new Formatter()).format("%10d", equationElapsedTime).toString() );
	    results.append(", ");

	    results.append( (new Formatter()).format("%10d", fileElapsedTime).toString() );
		
	    try {
	    	bufferedWriter.write(results.toString()); 
	    	bufferedWriter.newLine();
		} catch (IOException ex) {}
		
		results.setLength(0);
	}
	
	public void closeTimeLog() {
		try { 
			bufferedWriter.close(); 
		} catch (IOException ex) {}
	}
	
	public void printLogEntry(int iteraction) {
		System.out.printf("%d -> %d, %d, %d, %d, %d\n", iteraction, elapsedTime, iteractionElapsedTime, counterElapsedTime,
			              equationElapsedTime, fileElapsedTime);
	}
}

class Screen {
	public static final boolean SCREENING = true;
}