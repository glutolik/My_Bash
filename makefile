all:myBash

myBash:main.c app_running.c
	gcc main.c app_running.c -o myBash -std=c99

main.c:

app_running.c: