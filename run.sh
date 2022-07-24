#!/bin/bash

set -e

MAX_LOGS=20

echo "==== CMAKE INIT ===="
cmake .

echo "==== BUILD CACHE ===="
make

echo "==== PRUNE LOGS [Max: $MAX_LOGS] ===="
python3 prune_logs.py

echo "==== RUN CACHE ===="
#cvalgrind --tool=drd --trace-alloc=yes --trace-rwlock=yes --show-stack-usage=yes -s ./htfh_rt_search_cache
valgrind --tool=helgrind --delta-stacktrace=yes --ignore-thread-creation=yes --free-is-write=yes -s ./htfh_rt_search_cache
#cvalgrind --leak-check=full --show-leak-kinds=all --track-origins=yes -s ./htfh_rt_search_cache
