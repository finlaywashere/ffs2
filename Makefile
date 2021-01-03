CC = gcc

build:
	$(CC) -g -I. ffs2_utils.c -o ffs2_utils.o
