#!/bin/bash

echo "Testing $1 - merge - $2"

ALPHA=0.1
BETA=0.25
minSup=0.25

numClus=`wc -l $1/nat | cut -d ' ' -f 1`
echo "numClusters $numClus"

for i in `seq 0 4`; do
    echo $i
    python cluster.py $1/categories $1/data.txt $ALPHA $BETA $minSup $numClus $i $2 > $1/result$i
done

for i in `seq 0 4`; do
    echo $i
    python accuracy1.py $1/nat $1/result$i
done
