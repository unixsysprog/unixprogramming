#
# makefile for bounce program
#
#
bouncep: bounce.c
	cc -DPOLLED bounce.c -lcurses -o bouncep

bouncei: bounce.c alarmlib.o
	cc -DINTERRUPT bounce.c alarmlib.o -lcurses -o bouncei
