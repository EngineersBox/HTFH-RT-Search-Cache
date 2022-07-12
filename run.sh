#!/bin/bash

echo "==== CMAKE INIT ===="
cmake .

echo "==== BUILD CACHE ===="
make

echo "==== PRUNE LOGS ===="
log_count=$(ls logs | wc -l)
echo "Found $log_count logs"
if [ $log_count -gt 0 ]; then
  rm logs/*
  echo "Removed $log_count logs"
else
  echo "No logs to remove, skipping"
fi

echo "==== RUN CACHE ===="
cvalgrind --leak-check=full --show-leak-kinds=all --track-origins=yes -s ./htfh_rt_search_cache
