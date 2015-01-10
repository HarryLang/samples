#include <stdio.h>
#include <string.h>
#include <stdlib.h> // for calloc, free, exit
#include <mpi.h> // for MPI
#include <time.h> // to seed the random generator
#include <sys/time.h> // for gettimeofday
#define square(x) (x)*(x)
#define minimum(x,y) ((x < y) ? x : y)

/* We assume the number of vectors in a file to avoid dynamically building an array.
 * In Java we'd just use an ArrayList.
 * This is just for ease of implementation.
 * In a real implementation we would not make this assumption!
 */
#define NUM_VECTORS_PER_FILE 20000
#define DIM 3
#define NUM_ITERATIONS 10

/* These are tags for the MPI messages.
 * They indicate which task the worker should perform.
 */
#define INITIALIZE 1
#define UPDATE 2
#define FREE_MEMORY 3
#define LLOYD 4
#define TERMINATE 0

#define PRINT(s) {printf(s); fflush(stdout);}
// #define TESTING

#ifdef TESTING
int masterShouldPrint;
#endif // TESTING

int ierr; // for MPI calls
int numProcs; // for master only
float * pdf; //for workers only

struct WeightedVector {
    float weight;
    float x[DIM]; // the (x,y,z) coordinates
};

// prototypes
int reservoir(float priorWeight, float singleWeight);
float getRandom(float maxValue);
float squaredDistance(struct WeightedVector a, struct WeightedVector b);
void centerOfMass(struct WeightedVector* a, struct WeightedVector* b);
void worker_update(struct WeightedVector c);
void worker_initiate();
struct WeightedVector master_select();


struct WeightedVector* localData; // for workers only

/* Subtract the `struct timeval' values X and Y,
    storing the result in RESULT.
    Return 1 if the difference is negative, otherwise 0. */
void timeval_subtract (struct timeval * result, struct timeval * x, struct timeval * y)
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  //return x->tv_sec < y->tv_sec;
}

void loadData() {
    int myID; // used only to determine file name
    char* fileName;
    fileName = calloc(50, sizeof(char));
    FILE* myFile;
    char line[128];
    char* token;
    int numTokens = 4; //number of tokens
    int maxLL = 128; //maximum line length for fgets
    char buffer[2];
    //get the file id (same as worker id)
    ierr = MPI_Comm_rank(MPI_COMM_WORLD, &myID);
    myID--;
    strcat(fileName, "myDFS/part-0000");
    snprintf(buffer, 2, "%d", myID);
    strcat(fileName, buffer);

    myFile = fopen(fileName, "r");

    if (myFile == NULL) {
        perror("Error opening an input file");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < NUM_VECTORS_PER_FILE; i++) {
        //read a line and write it as a WeightedVector
        if (fgets(line, maxLL, myFile) != NULL) {
            token = strtok(line, " \n");
            localData[i].weight = strtof(token, NULL);
            // write it as a WeightedVector
            for (int j = 0; j < DIM; j++) {
                token = strtok(NULL, " \n");
                localData[i].x[j] = strtof(token, NULL);
            }
        } else {
            perror("Error reading a line from input file");
        }
    }
    fclose(myFile);
}

void master_lloyd(struct WeightedVector* centroids, int k) {

    // clear weights
    for(int i = 0; i < k; i++) {
        centroids[i].weight = 0;
    }

    struct WeightedVector temp[k]; // the message received

    for(int i = 1; i < numProcs; i++) {
        ierr = MPI_Recv(&temp, k*sizeof(struct WeightedVector), MPI_BYTE, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        for(int j = 0; j < k; j++) {
            centerOfMass(&centroids[j], &temp[j]);
        }
    }
}

/* Outline for masterNode algorithm:
   send INITIATE to all workers
   receive total weight of vectors from each worker and a winner per worker
   reservoir sample the list of winners to select the first centroid
   while (numCentroids < k) {
     send the centroid to the workers: (1) UPDATE message (2) WeightedVector
     receive partial sums and winners
     reservoir sample as receive
     save new centroid
   }
   send terminate
   write centroids to file and free the memory taken by centroids
*/
void masterNode(int k) {

    // runtime
    struct timeval ta, tb, tresult;
    gettimeofday ( &ta, NULL );



    int i; //loop index
    //create array that will hold centroids
    struct WeightedVector centroids[k];
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
    int cIndex = 0; // index of centroid to be computed

    // first iteration (from here until count++)
    centroids[0].weight = INITIALIZE;
    //ask all the workers to load the data and send back their total weights and winners
    ierr = MPI_Bcast(&centroids[0], sizeof(struct WeightedVector), MPI_BYTE, 0, MPI_COMM_WORLD);

    #ifdef TESTING
    masterShouldPrint = 1;
    #endif

    centroids[0] = master_select();
    cIndex++;

    //reservoir sample the responses from the workers and select the first centroid
    while (cIndex < k) {
        centroids[cIndex - 1].weight = UPDATE;
        ierr = MPI_Bcast(&centroids[cIndex - 1], sizeof(struct WeightedVector), MPI_BYTE, 0, MPI_COMM_WORLD);
        centroids[cIndex] = master_select();
        cIndex++;

    } // end while loop

    centroids[k].weight = FREE_MEMORY;

    ierr = MPI_Bcast(&centroids[k], sizeof(struct WeightedVector), MPI_BYTE, 0, MPI_COMM_WORLD);

    for(int i = 1; i <= NUM_ITERATIONS; i++) {
        centroids[0].weight = LLOYD;
        ierr = MPI_Bcast(&centroids, k*sizeof(struct WeightedVector), MPI_BYTE, 0, MPI_COMM_WORLD);
        master_lloyd(centroids, k); // sets the centroids to the new centers of mass
    }

    centroids[0].weight = TERMINATE;

    ierr = MPI_Bcast(&centroids[k], sizeof(struct WeightedVector), MPI_BYTE, 0, MPI_COMM_WORLD);

    for(int i = 0; i < k; i++) {
        printf("Centroid %d x-value: %f ", i+1, centroids[i].x[0]);
        printf("y = %f z = %f\n", centroids[i].x[1], centroids[i].x[2]);
    }

    gettimeofday ( &tb, NULL );

    timeval_subtract ( &tresult, &tb, &ta );

    printf("Runtime: %li microseconds\n", tresult.tv_sec * 1000000 + tresult.tv_usec);

    MPI_Finalize();

    exit(0);

}

struct WeightedVector master_select() {
    struct WeightedVector candidate; // next to check
    struct WeightedVector winner; // currently selected
    #ifdef TESTING
    struct WeightedVector candidate2;
    struct WeightedVector winner2;
    if (masterShouldPrint == 1) {
        masterShouldPrint = 0;
        ierr = MPI_Recv(&candidate, sizeof(struct WeightedVector), MPI_BYTE, 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        ierr = MPI_Recv(&winner, sizeof(struct WeightedVector), MPI_BYTE, 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        ierr = MPI_Recv(&candidate2, sizeof(struct WeightedVector), MPI_BYTE, 2, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        ierr = MPI_Recv(&winner2, sizeof(struct WeightedVector), MPI_BYTE, 2, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        centerOfMass(&winner, &winner2);
        centerOfMass(&candidate, &candidate2);
        printf("\nThe following two centroids are optimal only when you built the data with k = 2 and f = 2\n");
        printf("And if you called mpirun -np 3 cluster 2\n");
        printf("Optimal centroid: x = %f  y = %f  z = %f\n", winner.x[0], winner.x[1], winner.x[2]);
        printf("Optimal centroid: x = %f  y = %f  z = %f\n", candidate.x[0], candidate.x[1], candidate.x[2]);
    }
    #endif
    winner.weight = 0; // no probability
    for(int i = 1; i < numProcs; i++) {
        ierr = MPI_Recv(&candidate, sizeof(struct WeightedVector), MPI_BYTE, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (reservoir(winner.weight, candidate.weight)) {
            winner.x[0] = candidate.x[0];
            winner.x[1] = candidate.x[1];
            winner.x[2] = candidate.x[2];
        }
        winner.weight += candidate.weight;
    }
    return candidate;
}

int find_closest(struct WeightedVector* centroids, int k, struct WeightedVector x) {
    int closestIndex = 0;
    int closestDistance = squaredDistance(centroids[0], x);
    for(int i = 1; i < k; i++) {
        if(closestDistance > squaredDistance(centroids[i], x)) {
            closestDistance = squaredDistance(centroids[i], x);
            closestIndex = i;
        }
    }
    return closestIndex;
}

void worker_lloyd(struct WeightedVector* centroids, int k) {

    // the empty centroids for the next iteration
    struct WeightedVector temp[k];

    // clear weights - should be already 0
    for(int i = 0; i < k; i++) {
        temp[i].weight = 0;
    }

    for(int i = 1; i < NUM_VECTORS_PER_FILE; i++) {
        int closestIndex = find_closest(centroids, k, localData[i]);
        centerOfMass(&temp[closestIndex], &localData[i]);
    }

    ierr = MPI_Send(&temp, k*sizeof(struct WeightedVector), MPI_BYTE, 0, 0, MPI_COMM_WORLD);
}

void workerNode(int k) {
    localData = calloc(NUM_VECTORS_PER_FILE, sizeof(struct WeightedVector));

    loadData();

    srand(time(0));

    struct WeightedVector centroid; // new centroid from master node

    int seeding = 1;

    while(seeding) {
        ierr = MPI_Bcast(&centroid, sizeof(struct WeightedVector), MPI_BYTE, 0, MPI_COMM_WORLD);

        switch ((int) centroid.weight) {
            case INITIALIZE:
                worker_initiate();
                break;
            case UPDATE:
                worker_update(centroid);
                break;
            case FREE_MEMORY:
                free(pdf);
                seeding = 0;
                break;
            default:
                free(pdf);
                MPI_Finalize();
                exit(0);
        }
    }

    struct WeightedVector centroids[k];

    while (1) {

        ierr = MPI_Bcast(&centroids, k*sizeof(struct WeightedVector), MPI_BYTE, 0, MPI_COMM_WORLD);

        switch ((int) centroids[0].weight) {
            case LLOYD:
                worker_lloyd(centroids, k);
                break;
            default:
                MPI_Finalize();
                exit(0);
        }
    }
}

void worker_initiate() {
    float totalWeight = localData[0].weight; // the total weight of all local data points
    int winner = 0;

    #ifdef TESTING
    struct WeightedVector myFirst;
    struct WeightedVector mySecond;
    #endif

    for(int i = 1; i < NUM_VECTORS_PER_FILE; i++) {
        if (reservoir(totalWeight, localData[i].weight)) {
            winner = i;
        }
        #ifdef TESTING
        if (localData[i].x[0] > 125) {
            centerOfMass(&mySecond, &localData[i]);
        } else if (localData[i].x[0] > 50) {
            printf("TROUBLE");
        } else {
            centerOfMass(&myFirst, &localData[i]);
        }
        #endif

        totalWeight += localData[i].weight;
    }

    #ifdef TESTING
    ierr = MPI_Send(&myFirst, sizeof(struct WeightedVector), MPI_BYTE, 0, 1, MPI_COMM_WORLD);
    ierr = MPI_Send(&mySecond, sizeof(struct WeightedVector), MPI_BYTE, 0, 2, MPI_COMM_WORLD);
    #endif

    struct WeightedVector result;
    result.weight = totalWeight;
    result.x[0] = localData[winner].x[0];
    result.x[1] = localData[winner].x[1];
    result.x[2] = localData[winner].x[2];

    ierr = MPI_Send(&result, sizeof(struct WeightedVector), MPI_BYTE, 0, 0, MPI_COMM_WORLD);

    pdf = calloc(NUM_VECTORS_PER_FILE, sizeof(float));
    pdf[0] = -1; // to signify that the list hasn't been initialized

}

void worker_update(struct WeightedVector c) {
    int winner = 0;
    float totalWeight = 0;
    if (pdf[0] != -1) { // if not the first run
        pdf[0] = minimum(pdf[0], squaredDistance(localData[0], c));
        totalWeight = localData[0].weight * pdf[0]; // the total weight of all local data points
        for(int i = 1; i < NUM_VECTORS_PER_FILE; i++) {
            pdf[i] = minimum(pdf[i], squaredDistance(localData[i], c));

            if (reservoir(totalWeight, localData[i].weight * pdf[i])) {
                winner = i;
            }
            totalWeight += localData[i].weight * pdf[i];
        }
    } else { // if the first run

        pdf[0] = squaredDistance(localData[0], c);

        totalWeight = localData[0].weight * pdf[0];

        for(int i = 1; i < NUM_VECTORS_PER_FILE; i++) {
            pdf[i] = squaredDistance(localData[i], c);
            if (reservoir(totalWeight, localData[i].weight * pdf[i])) {
                winner = i;
            }
            totalWeight += localData[i].weight * pdf[i];
        }
    }

    struct WeightedVector result;
    result.weight = totalWeight;
    result.x[0] = localData[winner].x[0];
    result.x[1] = localData[winner].x[1];
    result.x[2] = localData[winner].x[2];

    ierr = MPI_Send(&result, sizeof(struct WeightedVector), MPI_BYTE, 0, 0, MPI_COMM_WORLD);
}

/* reservoir sampling
 * priorWeight = weight of the stream so far
 * singleWeight = weight of the candidate point to be selected
 */
int reservoir(float priorWeight, float singleWeight) {
    if (getRandom(priorWeight + singleWeight) < singleWeight) {
        return 1;
    } else {
        return 0;
    }
}

// returns a random float between 0 and maxValue
float getRandom(float maxValue) {

    float myRand = ((float) rand() )/ (RAND_MAX / maxValue);
    return myRand;

}

// returns the squared Euclidean distance between a and b
float squaredDistance(struct WeightedVector a, struct WeightedVector b) {
    float squaredDistance = 0;
    for(int i = 0; i < DIM; i++) {
        squaredDistance += square(a.x[i] - b.x[i]);
    }
    return squaredDistance;
}

// changes a to be the weighted average of a and b
void centerOfMass(struct WeightedVector* a, struct WeightedVector* b) {

    for(int i = 0; i < DIM; i++) {
        a->x[i] = a->weight*a->x[i] + b->weight*b->x[i];
        a->x[i] /= (a->weight + b->weight);
    }

    a->weight += b->weight;
}

int main(int argc, char **argv) {
    ierr = MPI_Init(&argc, &argv);
    int myID;

    MPI_Comm_rank (MPI_COMM_WORLD, &myID);

    int k = atoi(argv[1]); // only master needs to know

    if(myID != 0) {
        workerNode(k);
    } else {
        masterNode(k);
    }

    // MPI will finalize and exit in the above loop, but just in case:
    MPI_Finalize();
    exit(0);
}
