import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/*
 * FUNCTION: This program looks for "triangle.txt"
 * and computes the value of the maximum path.
 * 
 * RUNTIME: The naive approach is O(2^n).  This algorithm
 * is O(n^2) by using dynamic programming techniques.
 * 
 * Author: Harry Lang
 * Date: 30 March 2014
 */

public class triangle {
	
	// create array to store triangle
	static List<Integer[]> triangle = new ArrayList<Integer[]>();
	
	// loads data from "triangle.txt" into the array "triangle"
	private static void loadArray() {	
		
		try {			
			BufferedReader reader = null;
			
			try {							
				reader = new BufferedReader(new FileReader("triangle.txt"));
				
				int line = 0;
				String currentLine = reader.readLine();
						
				// while the file has more lines
				while(currentLine != null) {
					
					// tokenize line on spaces
					String[] eachEntry = currentLine.split(" ");
					
					// make sure there are the correct number of entries
					if (eachEntry.length != line + 1) {
						System.out.println("Unexpected number of entries on line " + (line + 1) + ".");
						System.exit(1);
					}
					
					// create an array for the new row
					Integer[] eachInteger = new Integer[line + 1];
					
					for(int i = 0; i <= line; i++) {
						try {
							eachInteger[i] = Integer.parseInt(eachEntry[i]);
						} catch (NumberFormatException e) {
							System.out.println("On line " + (line + 1) + ", \"" + eachEntry[i] + "\" is not a valid integer.");
							System.exit(1);
						}						
					}
					
					// add the new row to triangle
					triangle.add(eachInteger);
					
					line++;
					currentLine = reader.readLine();
				}
			} finally {
				if (reader != null) {
					reader.close();
				}
			}
		} catch (IOException e) {
			System.out.println("The file \"triangle.txt\" was not found.");
			System.exit(1);
		}
		
		// if the file was empty
		if (triangle.size() <= 0) {
			System.out.println("The file was empty.");
			System.exit(1);
		}
	}
	
	public static void main(String args[]) {
		
		long startTime = System.currentTimeMillis();

		// exits here if failure to load data from file
		loadArray();
		
		// start at second row to bottom
		for(int row = triangle.size() - 2; row >= 0; row--) {
			for(int column = 0; column <= row; column++) {
				int toLeft = triangle.get(row + 1)[column];
				int toRight = triangle.get(row + 1)[column + 1];
				triangle.get(row)[column] += ((toLeft > toRight) ? toLeft : toRight);
			}
		}
		
		System.out.println("The maximum total is " + triangle.get(0)[0]);	

		long endTime = System.currentTimeMillis();
		System.out.println("Runtime: " + (endTime - startTime) + " ms");
	}
}
