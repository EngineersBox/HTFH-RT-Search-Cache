#!/bin/bash

MAX_LOGS=20

echo "==== CMAKE INIT ===="
cmake .

echo "==== BUILD CACHE ===="
make

echo "==== PRUNE LOGS [Max: $MAX_LOGS] ===="
python3 prune_logs.py

echo "==== RUN CACHE ===="
cvalgrind --leak-check=full --show-leak-kinds=all --track-origins=yes -s ./htfh_rt_search_cache
