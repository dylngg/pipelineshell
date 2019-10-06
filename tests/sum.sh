#!/bin/sh
# Sums the numbers in stdin (seperated by newline)
total=0
while IFS='$\n' read -r line; do
    ((total+=$line))
done
echo $total
