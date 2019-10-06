#!/bin/bash
# Prints useful information about a process by name

# Get the all the processes by that name
pids=`pgrep $1`

# Loop through the processes
for pid in $pids; do
    status="cat /proc/$pid/status"

    # For each desired field
    for field in ^Name: ^Pid: ^PPid: ^Uid: ^VmRSS; do

        # Find that line in the status file, eliminate unneccesary whitespace
        # and replace the first whitespace with a tab to line things up nicely
        $status | grep "$field" | xargs | sed -e $'s/ /\t /'
    done
done
