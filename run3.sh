#!/bin/bash
date

nohup taskset -c 3 ./run.sh &

nohup taskset -c 2 ./run2.sh &
wait
date
