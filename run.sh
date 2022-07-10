cmake .
make
colour-valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes -s ./htfh_rt_search_cache
