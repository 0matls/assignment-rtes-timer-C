#!/bin/bash

# Run timer with arguments 1, 2, 3, 4
for ((i=1; i<=4; i++))
do
    echo "Running timer with argument: $i"
    ./timer $i
done

echo "Timer execution complete."
