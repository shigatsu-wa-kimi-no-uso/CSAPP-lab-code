CC = gcc
CFLAGS = -g -Wall -Werror -std=c99 -m64

csim: csim.c cachelab.c cachelab.h
	$(CC) $(CFLAGS) -o csim csim.c cachelab.c argparser.c cache.c opparser.c simulator.c -lm 