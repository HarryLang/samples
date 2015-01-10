import java.io.IOException;

import org.apache.hadoop.io.*;
import org.apache.hadoop.mapreduce.*;

public class Reduce extends Reducer<Text, IntWritable, Text, Text> {
    private final static Text blank = new Text("");

    public void reduce(Text key, Iterable<IntWritable> values, Context context) throws IOException, InterruptedException {
      int count = 0;
      for (IntWritable value : values) {
        count += 1;
      }

      // count should never be > 3
      if (count >= 3) {
        //printTriangle(key, context);
        String[] itr = key.toString().split("\\s+");
        //output.collect(key, null);
        if (itr.length < 3) { return; }
        Text t1 = new Text();
        Text t2 = new Text();
        Text t3 = new Text();
        t1.set("<" + itr[0] + "," + itr[1] + "," + itr[2] + ">");
        t2.set("<" + itr[1] + "," + itr[0] + "," + itr[2] + ">");
        t3.set("<" + itr[2] + "," + itr[0] + "," + itr[1] + ">");
        context.write(t1, blank);
        context.write(t2, blank);
        context.write(t3, blank);
      }
    }
  }
