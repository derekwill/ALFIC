all: AFLIC.c AFLIC.h
	gcc -o AFLIC -lpthread AFLIC.c
