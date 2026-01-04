#!/usr/bin/bash
#for dir in task1 task2 task3 task4 bonus; do
#  (cd "$(dirname "$0")/${dir}" && bash build.sh -p sw) &
#done

for dir in task1 task2 task3 task4 bonus; do
  (cd "$(dirname "$0")/${dir}" && python process_logs.py -p sw && python plot.py -p sw && python process_logs.py -p hw && python plot.py -p hw) &
done

wait
