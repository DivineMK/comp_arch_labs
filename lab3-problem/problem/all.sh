#!/usr/bin/bash
for dir in task2 task3 task4; do
  (cd $(dirname "$0")/${dir} && bash build.sh) &
done

wait
