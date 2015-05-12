all:myBash

myBash:main.c app_running.c
	gcc main.c app_running.c -o myBash -std=c99 -lrt

main.c:

app_running.c: