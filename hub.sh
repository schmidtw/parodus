#!/bin/bash

# Start the first process
parodus $@ >> parodus.txt 2>&1 &
status=$?
if [ $status -ne 0 ]; then
  echo "Failed to start my_first_process: $status"
  exit $status
fi

sleep 10
# Start the second process
./tests/printer -p tcp://127.0.0.1:6666 -c tcp://127.0.0.1:9001 -t 10000 -n printer1 -m 4bb161000b60 >> printer.txt 2>&1 &
status=$?
if [ $status -ne 0 ]; then
  echo "Failed to start my_second_process: $status"
  exit $status
fi

# Naive check runs checks once a minute to see if either of the processes exited.
# This illustrates part of the heavy lifting you need to do if you want to run
# more than one service in a container. The container exits with an error
# if it detects that either of the processes has exited.
# Otherwise it loops forever, waking up every 60 seconds

sleep 1

tail -f parodus.txt
