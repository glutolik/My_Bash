#ifndef CALLS_H_INCLUDED
#define CALLS_H_INCLUDED

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "app_running.h"

int oneProgPars(char*** output, const char* callstr);

int oneStrCall(const char* callstr, char** path);

int scriptRunner(int infd, int outfd, char* name, char** argv);

#endif // CALLS_H_INCLUDED
