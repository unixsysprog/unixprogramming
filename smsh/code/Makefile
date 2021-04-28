#
# Makefile for smsh
#
# Type  make to compile the smsh program
#

CC = gcc -g -Wall

smsh: smsh.o splitline.o execute2.o process2.o controlflow.o \
		dllist.o builtin.o flexstr.o  varlib.o
	$(CC) -o smsh smsh.o splitline.o execute2.o process2.o \
		controlflow.o dllist.o builtin.o  flexstr.o varlib.o

controlflow.o : controlflow.c smsh.h dllist.h
	$(CC) -c controlflow.c

dllist.o : dllist.c dllist.h
	$(CC) -c dllist.c

clean: 
	rm -f *.o core
