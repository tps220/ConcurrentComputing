#!/bin/bash
threads="1 2 4 8 12 16 20 24"

for thread in ${threads}
do
    obj32/p1 -t ${thread} -i 100000 -b 2048 -k 100000
done
