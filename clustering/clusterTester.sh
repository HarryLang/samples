#!/bin/bash

# outputs average runtimes for clustering program "cluster"

numRuns=20
kMin=5
kMax=20
kStep=5 # the amount to increment by

# the number of worker nodes
pMin=1
pMax=4

for ((k=$kMin; k <= $kMax; k = $k + $kStep))
do
  for ((p=$pMin; p <= $pMax; p++ ))
  do
    temp=0
    for ((i=1; i<=$numRuns; i++))
    do
      clusterCommand="mpirun -np $[$p+1] cluster $k"
      runtime=$($clusterCommand | grep "Runtime" | grep -o [0-9]*)
      ((temp=$temp+$runtime))
    done
    ((average=$temp/$numRuns))
    echo "k = $k, p = $p: $average microseconds"
  done
done
