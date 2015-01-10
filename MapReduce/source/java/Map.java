import java.io.IOException;
import java.util.*;

import org.apache.hadoop.io.*;
import org.apache.hadoop.mapreduce.*;

public class Map extends Mapper<LongWritable, Text, Text, IntWritable> {
  private Text angle = new Text();
  private int vertex;
  private final static IntWritable one = new IntWritable(1);
  
  // returns the three elements in order
  private String sortAngle(int i, int j, int[] friends) {
    int[] angleArray = {vertex, friends[i], friends[j]};
    Arrays.sort(angleArray);
    String orderedList = Integer.toString(angleArray[0]) + " " + Integer.toString(angleArray[1]) + " " + Integer.toString(angleArray[2]);
    return orderedList;
  }
  
  public void map(LongWritable key, Text value, Context context) throws IOException, InterruptedException {
    String line = value.toString();
    String[] itr = line.split("\\s+"); //split on whitespace
    // if the line was empty
    if (itr.length == 0) { return; }
    vertex = Integer.parseInt(itr[0]); 
    int[] friends = new int[itr.length-1]; 
    
    // load the array
    for (int k = 0; k < friends.length; k++) {
      friends[k] = Integer.parseInt(itr[k+1]);
    }
    
    // calculate all N-choose-2 pairs
    for (int i = 0; i < friends.length - 1; i++) { // friends.length - 1, since need room for j
      for(int j = i + 1; j < friends.length; j++) {
        angle.set(sortAngle(i, j, friends));
        context.write(angle, one);
      }
    }
  }
}