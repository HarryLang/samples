#!/bin/bash

# help function
help_user()
{
  echo "Usage:"
  echo "-f: the number of files to spread the data across"
  echo "-n: the number of datapoints per file"
  echo "-d: the dimensionality of the data"
  echo "-k: the number of clusters to create"
  exit 1
}

# Check that $1 is an integer >= 1
# $1 = variable name, $2 = flag name
check_positive_integer()
{
if (! echo $1 | egrep -q '^[0-9]+$') || (echo $1 | egrep -q '^0+$')
then
  echo "The -$2 flag must be followed by a positive integer"
  help_user
  exit 1
fi
}

# write 4 random files of size 1000 in Euclidean 3-space
# generates data in 10 disjoint hypercubes of radius 100

numFiles=4
numVectors=20000
dimension=3
numClusters=5

# the cluster radius (generated in hypercube)
R=50

while getopts ":n:d:f:k:" opt
do
  case "$opt" in
    f)
      numFiles="$OPTARG"
      check_positive_integer $numFiles f
      ;;
    n)
      numVectors="$OPTARG"
      check_positive_integer $numVectors n
      ;;
    d)
      dimension="$OPTARG"
      check_positive_integer $dimension d
      ;;
    k) 
      numClusters="$OPTARG"
      check_positive_integer $numClusters k
      ;;
    \?) help_user;;
  esac
done

# the -p makes it work even if myDFS already exists
mkdir -p myDFS

echo "Generating $numFiles files, each with $numVectors weighted vectors in Euclidean $dimension-space"
echo "Data is being sampled from $numClusters disjoint hypercubes"

for (( i = 0; i < $numFiles; i++ ))
do
  ((s=$i + 1))
  echo "Generating file $s of $numFiles..."
  
  fileName="./myDFS/part-0000$i"
  
  # clear contents of file, if it isn't empty
  > $fileName

  for (( j = 0; j < $numVectors; j++ ))
  do
    # the weight is between 1 and 25
    ((weight=($RANDOM % 25) + 1))
    ((fraction=$RANDOM%100))
    line="$weight.$fraction"

    # each cluster is in a cube about a different X value
    ((centerX=(5*$R)*(($RANDOM%$numClusters)-(($numClusters-1)/2))))
    ((x=centerX + ($RANDOM % $R) - ($R / 2)))
    ((fraction=$RANDOM%100))

    line="$line $x.$fraction"

    # the other dimensions
    for (( k = 2; k <= $dimension; k++ ))
    do
      ((x=($RANDOM % $R) - ($R / 2)))
      ((fraction=$RANDOM%100))
      # concatenate
      line="$line $x.$fraction"
    done

    echo $line >> $fileName
  done
done

echo "All files successfully generated"

exit 0
