#!/bin/bash

# Function to kill processes
cleanup() 
{
    echo "Stopping the web server and receiver..."
    kill $PID1 $PID2
    wait $PID1 $PID2 2>/dev/null
}

# Activate the first virtual environment and start the web server
source ./env/bin/activate

python ./RPi/wserver.py &
PID1=$!

# Activate the second virtual environment and start the receiver script
source ./env/bin/activate

python ./RPi/receiver.py &
PID2=$!

# Setup trap for cleanup on CTRL+C
trap cleanup INT

# Wait for processes to exit
wait $PID1 $PID2

# Cleanup action
cleanup
