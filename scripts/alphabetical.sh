#!/bin/bash

array=("banana" "apple" "cherry" "mango" "blueberry")

sorted_array=($(for i in "${array[@]}"; do echo $i; done | sort))

echo "Sorted array:"
for i in "${sorted_array[@]}"; do
    printf "%s, " $i
done
