#!/bin/sh
set -e

cd /data/test

start_screen_app() {
    # for DAYU200, start the app at front-end
    # to keep the screen on
    echo -e "\033[32mStarting screen app...\033[0m"
    aa start -a EntryAbility -b com.example.myapplication
    sleep 3
    echo -e "\033[32mScreen app started\033[0m"
}

# check arguments
if [ $# -ne 1 ]; then
    echo -e "\033[31mError: Invalid arguments\033[0m"
    echo -e "\033[31mUsage: ./test.sh [run|stop|clean]\033[0m"
    exit 1
elif [ $1 = "run" ]; then
    start_screen_app
    
    # if there is at least 2 file /dev/ttyUSB*(not always named as 0 or 1)
    # we can start our working processes
    # otherwise, we should report error
    # then wait for the device to be connected
    echo -e "\033[32mChecking USB devices...\033[0m"
    numUSB=$(ls /dev/ttyUSB* | wc -l)
    if [ $numUSB -ne 2 ]; then
        echo -e "\033[31mError: Failed to check USB devices\033[0m"
        exit 1
    fi
    
    # start our working processes
    echo -e "\033[32mStarting our programs...\033[0m"
    nohup ./lidar_driver > lidar.log 2>&1 &
    nohup ./navigation > navi.log 2>&1 &
    nohup ./serial > serial.log 2>&1 &
    nohup ./udp2lcm > udp2lcm.log 2>&1 &
    sleep 1
    echo -e "\033[32mPrograms started\033[0m"
    
    echo hello
elif [ $1 = "newrun" ]; then
    start_screen_app
    
    # if there is at least 2 file /dev/ttyUSB*(not always named as 0 or 1)
    # we can start our working processes
    # otherwise, we should report error
    # then wait for the device to be connected
    echo -e "\033[32mChecking USB devices...\033[0m"
    numUSB=$(ls /dev/ttyUSB* | wc -l)
    if [ $numUSB -ne 2 ]; then
        echo -e "\033[31mError: Failed to check USB devices\033[0m"
        exit 1
    fi
    
    # start our working processes
    echo -e "\033[32mStarting our programs...\033[0m"
    nohup ./lidar_driver > lidar.log 2>&1 &
    nohup ./serial > serial.log 2>&1 &
    nohup ./udp2lcm > udp2lcm.log 2>&1 &
    sleep 1
    echo -e "\033[32mPrograms started\033[0m"
    
    echo hello
elif [ $1 = "stop" ]; then
    # kill the processes one by one
    # echo -e "\033[32mStopping our programs...\033[0m"
    set +e
    pids=$(ps -ef | grep -E "lidar_driver|navigation|serial|udp2lcm|python" | grep -v grep | ./getpid)
    for i in $pids; do
        kill -2 $i
    done
    set -e
    sleep 2
    echo -e "\033[32mStopped!!!\033[0m"
elif [ $1 = "clean" ]; then
    # clean the log files
    echo -e "\033[32mCleaning log files...\033[0m"
    rm -f *.log de*
    echo -e "\033[32mCleaned!!!\033[0m"
else
    echo -e "\033[31mError: Invalid arguments\033[0m"
    echo -e "\033[31mUsage: ./test.sh [run|newrun|stop|clean]\033[0m"
    exit 1
fi
