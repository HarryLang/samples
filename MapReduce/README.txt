- Upload the necessary files to the S3 bucket
- Click "Create Cluster" on EMR
- Set the log folder to s3://hlang8/logs/
- Remove the default option of installing Hive and Pig
- Have the master be m1.small, and select a type/# of workers
- Select "Streaming program" or "Custom JAR" for the step
- Click "Configure and add"
- Fill in the appropriate fields (see * below), ensuring that the output directory is not yet created (this will cause a failure)
- For "Action on failure", select "Terminate cluster"
- Click "Add"
- Select "Yes" for "Auto-terminate"
- Click "Create cluster"
- Get cup of tea and wait

* My exact entries for configuring the streaming program:
Mapper			s3://hlang8/streaming-jobs/mapper.py
Reducer			s3://hlang8/streaming-jobs/reducer.py
Input				s3://friends1000/
Output			s3://hlang8/output/friends_1000/

* My exact entries for configuring the custom JAR:
JAR S3 location		s3://hlang8/jobs/triangles.jar
Arguments			Triangles
				s3://friends1000/
				s3://hlang8/output/friends_1000/
