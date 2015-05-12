all: e-bash

e-bash: main.o app_running.o
	gcc -o e-bash main.o app_running.o -std=c99 -lrt

main.o: main.c app_running.c app_running.h
	gcc -c main.c -std=c99

app_running.o: app_running.c app_running.h
	gcc -c app_running.c -std=c99

main.c:

app_running.c:

app_running.h:

