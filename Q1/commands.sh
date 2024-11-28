#!/bin/bash
sudo apt-get install python3-dev
pip3 install setuptools numpy matplotlib

echo -e "Compiling system_monitor.c \n"
gcc -o system_monitor system_monitor.c

echo -e "\nRunning setup.py \n"
python3 setup.py build_ext --inplace

echo -e "\nRunning system_monitor for 30 seconds...\n"

./system_monitor &
MONITOR_PID=$!

for i in {30..1}; do
    echo -ne "Time remaining: $i seconds\r"
    sleep 1
done
echo -e "\nTime's up! Stopping system monitor..."

kill $MONITOR_PID

echo -e "\nRunning metrics_plotter.py \n"
python3 metrics_plotter.py

echo -e "\nAll done! Check the generated PNG file for the plots."