all: e-bash

e-bash: main.o app_running.o calls.o
	gcc -o e-bash main.o app_running.o calls.o -std=c99 -lrt

main.o: main.c app_running.c app_running.h calls.c calls.h
	gcc -c main.c -std=c99

app_running.o: app_running.c app_running.h
	gcc -c app_running.c -std=c99

calls.o: calls.c app_running.c app_running.h
	gcc -c calls.c -std=c99

main.c:

app_running.c:

app_running.h:

calls.c:

calls.h:


