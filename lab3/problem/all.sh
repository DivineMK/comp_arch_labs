#!/usr/bin/bash
#for dir in task1 task2 task3 task4 bonus; do
#  (cd "$(dirname "$0")/${dir}" && bash build.sh -p sw) &
#done

for dir in task1 task2 task3 task4 bonus; do
  (cd "$(dirname "$0")/${dir}" && python plot.py -p hw) &
done

wait
