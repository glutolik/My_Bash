#ifndef CALLS_H_INCLUDED
#define CALLS_H_INCLUDED

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <limits.h>
#include "app_running.h"

#ifndef PATH_MAX //because i have not it in my limits.h
#define PATH_MAX 4096
#endif // PATH_MAX


int oneProgPars(char*** output, const char* callstr);

int oneStrCall(const char* callstr, char* path, JobsList* jobs, int infdFrom);

int scriptRunner(char** argv);

#endif // CALLS_H_INCLUDED
