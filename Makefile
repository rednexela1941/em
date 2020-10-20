CC=gcc
all: em
CFLAGS=-g -lreadline

em: em.c
	$(CC) -o em em.c $(CFLAGS)


