#!/bin/bash

MAX_LOGS=20

echo "==== CMAKE INIT ===="
cmake .

echo "==== BUILD CACHE ===="
make

echo "==== PRUNE LOGS [Max: $MAX_LOGS] ===="
# shellcheck disable=SC2012
LOG_COUNT=$(ls logs | wc -l)
if [ "$LOG_COUNT" -gt $MAX_LOGS ]; then
  rm logs/*
  echo "Removed $LOG_COUNT logs"
else
  echo "Found $LOG_COUNT logs, below threshold. Skipping."
fi

echo "==== RUN CACHE ===="
cvalgrind --leak-check=full --show-leak-kinds=all --track-origins=yes -s ./htfh_rt_search_cache
