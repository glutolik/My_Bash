#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include "app_running.h"

int outErr(int n)
{
    if (n != 0)
        fprintf(stderr, "ERROR: errno = %d\n", errno);
    return n;
}
int pathAcc(const char* path, const char* file)
{
    char fullPath[256];
    strcpy(fullPath, path);
    strcat(fullPath, file);
    return access(fullPath, F_OK);
}
void strspcpy(char* output, const char* from)
{
    int i = 0;
    while (from[i] > ' ')
    {
        output[i] = from[i];
        ++i;
    }
    output[i] = 0;
}
int progSepar(char ch)
{
    if (ch != '|' && ch != '&' && ch != 0 && ch != '(' && ch != ')')
        return -1;
    else
        return 0;
}
int oneProgPars(char*** output, const char* callstr)
{
    while (progSepar(*callstr) == -1 && *callstr <= ' ')
            ++callstr;
    int argc = 0;
    int i = 0;
    while (progSepar(callstr[i]) != 0)
    {
        while (callstr[i] > ' ')
        {
            if (callstr[i] == '\"' && (i == 0 || callstr[i - 1] != '\\'))
            {
                ++i;
                while (callstr[i] != '\"' && progSepar(callstr[i]) != 0)
                    ++i;
            }
            ++i;
        }
        while (progSepar(callstr[i]) != 0 && callstr[i] <= ' ')
            ++i;
        ++argc;
    }
    *output = (char**)malloc((argc + 1)*sizeof(char*));
    (*output)[argc] = NULL;
    int argnmb = 0;
    i = 0;
    while (progSepar(callstr[i]) != 0)
    {
        int arglen = 0;
        int reallen = 0;
        while (callstr[i + arglen] > ' ')
        {
            if (callstr[i + arglen] == '"' && (i + arglen == 0 || callstr[i + arglen - 1] != '\\'))
            {
                ++arglen;
                while (callstr[i + arglen] != '"' && progSepar(callstr[i + arglen] != 0))
                {
                    ++arglen;
                    ++reallen;
                }
                if (progSepar(callstr[i + arglen]) != 0)
                    --reallen;
            }
            ++arglen;
            ++reallen;
        }
        (*output)[argnmb] = (char*)malloc((reallen + 1)*sizeof(char));
        int j = 0;
        reallen = 0;
        for (j = i; j < i + arglen; ++j)
        {
            if (callstr[j] != '"' || (i != 0 && callstr[j - 1] == '\\'))
            {
                (*output)[argnmb][reallen] = callstr[j];
                ++reallen;
            }
        }
        (*output)[argnmb][reallen] = 0;
        //printf("ps %d: %s\n", argnmb, (*output)[argnmb]);
        i = i + arglen;
        while (progSepar(callstr[i]) != 0 && callstr[i] <= ' ')
            ++i;
        ++argnmb;
    }
    return argc;
}
int main(int argc, char **argv)
{
    char callstr[256];
    char file_addr[256];
    char path[256];
    getcwd(path, sizeof(path));
    int code;

    int i = 0;
    printf("\n");
    for (i = 0; i < 6; ++i)
    {
        printf("\033[%dm*\033[0m", 31 + i);
    }
    printf("\nHello in e-bash!\n");

    while (1)
    {
        printf("[\033[32m%s\033[0m]:%s> ", getenv("USER"), path);
        fflush(stdout);
        i = 0;
        char nch = getchar();
        while (i < 255 && nch != '\n')
        {
            callstr[i] = nch;
            nch = getchar();
            ++i;
        }
        callstr[i] = 0;
        char comName[256];
        i = 0;
        while (callstr[i] > ' ')
            ++i;
        strncpy(comName, callstr, i);
        comName[i] = 0;
        if (code == EOF || strcmp(comName, "exit") == 0) //и аналогично для всех внутренних команд
        {
            int i = 0;
            printf("\n");
            for (i = 0; i < 6; ++i)
            {
                printf("\033[%dm*\033[0m", 31 + i);
                fflush(stdout);
            }
            printf("\nGoodbye.\n");
            return 0;
        }
        else if (strcmp(comName, "cd") == 0)
        {
            if (callstr[2] != ' ')
            {
                printf("use cd as \"cd <directory>\" or same\n");
                printf("You can see \"help\", which hasn't realized yet\n");
                continue;
            }
            char newDir[256];
            int i = 2;
            while (callstr[i] <= ' ' && callstr[i] != 0)
                ++i;
            strcpy(newDir, callstr + i);
            int code = chdir(newDir);
            if (outErr(code) == 0)
                getcwd(path, sizeof(path));
        }
        else
        {
            //sprintf("%s isn't a internal comand\n", comName);

            int code = 0;
            int i = 0;
            //int len = strlen(callstr);
            char*** allsargv = NULL;
            char** progNames = NULL;
            int progCount = 1;
            char* nextProg = callstr;
            while(callstr[i] != 0)
            {
                if (callstr[i] == '|')
                    ++progCount;
                ++i;
            }
            allsargv = (char***)malloc((progCount)*sizeof(char**));
            progNames = (char**)malloc((progCount)*sizeof(char*));
            for (i = 0; i < progCount; ++i)
            {
                oneProgPars(allsargv + i, nextProg);
                progNames[i] = (char*)malloc((strlen(allsargv[i][0]) + 1)*sizeof(char));
                strcpy(progNames[i], allsargv[i][0]);
                progNames[i][strlen(allsargv[i][0])] = 0;
                while(*nextProg != 0 && *nextProg != '|')
                    ++nextProg;
                while(*nextProg != 0 && progSepar(*nextProg) == 0 || *nextProg <= ' ')
                    ++nextProg;
            }
            if (run_comand_chain(0, 1, 2, progCount, progNames, allsargv, &code) == -1)
            {
                printf("I can't find this comand: %s\n", comName);
                printf("You can see \"help\", which hasn't realized yet\n");
                continue;
            }
            for (i = 0; i < progCount; ++i)
            {
                int j = 0;
                while(allsargv[i][j] != NULL)
                {
                    free(allsargv[i][j]);
                    ++j;
                }
                free(progNames[i]);
                free(allsargv[i]);
            }
        }
    }
    /*void *tmp = NULL;
    wait(tmp);
    printf("%d\n", a);*/
    return 0;
}
