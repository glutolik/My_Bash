#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include "app_running.h"

int outErr(const char* text, int n)
{
    if (n != 0)
        fprintf(stderr, "ERROR in %s: errno = %d\n", text, errno);
    return n;
}
int mygch()
{
    struct termios tty, savetty;
    tcgetattr(0, &tty);
	savetty = tty;
	tty.c_lflag &= ~(ICANON | ECHO);
	tty.c_cc[VMIN] = 1;
	tcsetattr(0, TCSAFLUSH, &tty);
    int ch;
	read(0, &ch, sizeof(int));
	tcsetattr(0, TCSANOW, &savetty);
	return ch;
}


int maxCallLen = 512;

int loadHistory(char*** oldhist)
{
    char histpath[255];
    sprintf(histpath, "/home/%s/.e-bash_history", getenv("USER"));
    int oldhistfd = open(histpath, O_RDONLY);
    int strCount = 0;
    if (oldhistfd > 0)
    {
        struct stat st;
        fstat(oldhistfd, &st);
        char* mapedhist = (char*)mmap(NULL, st.st_size*sizeof(char), PROT_READ, MAP_SHARED, oldhistfd, 0);
        int i = 0;
        for (i = 0; i < st.st_size; ++i)
            if (mapedhist[i] == '\n')
                ++strCount;
        (*oldhist) = (char**)malloc(strCount*sizeof(char*));
        int pos = 0;
        for (i = 0; i < strCount; ++i)
        {
            int j = pos;
            while (j < st.st_size && mapedhist[j] != '\n')
                ++j;
            (*oldhist)[i] = (char*)malloc((j - pos + 1)*sizeof(int));
            strncpy((*oldhist)[i], mapedhist + pos, j - pos);
            (*oldhist)[i][j - pos] = 0;
            pos = j + 1;
        }
        munmap(mapedhist, st.st_size*sizeof(char));
        close(oldhistfd);
    }
    return strCount;
}
void appendHistory(const char** coms, int comCount)
{
    char histpath[255];
    sprintf(histpath, "/home/%s/.e-bash_history", getenv("USER"));
    FILE* histfile = fopen(histpath, "a");
    int i = 0;
    for (i = 0; i < comCount; ++i)
    {
        fprintf(histfile, "%s\n", coms[i]);
    }
    fclose(histfile);
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
    if (ch != '|' && ch != '&' && ch != 0 && ch != '(' && ch != ')' && ch != '>')
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
    char callstr[maxCallLen];
    char file_addr[256];
    char path[256];
    getcwd(path, sizeof(path));
    int code;

    int i = 0;
    printf("\n");
    for (i = 5; i >= 0; --i)
    {
        printf("\033[%dm*\033[0m", 31 + i);
    }
    printf("\nHello in e-bash!\n");

    char** oldhist = NULL;
    char** newhist = (char**)malloc(500*sizeof(char*));
    for (i = 0; i < 500; ++i)
        newhist[i] = (char*)malloc(maxCallLen*(sizeof(char)));
    int oldhistCount = loadHistory(&oldhist);
    int newhistCount = 0;
    int histPos = oldhistCount;
    struct winsize ws;
    ioctl(1, TIOCGWINSZ, &ws);
    int termWidth = ws.ws_col;

    while (1)
    {
        printf("[\033[32m%s\033[0m]:%s> ", getenv("USER"), path);
        fflush(stdout);
        i = 0;
        int len = 0;
        callstr[0] = 0;
        int ch = mygch();
        while ((char)ch != '\n' && (char)ch != EOF)
        {
            int i;
            int oldlen = len + strlen(getenv("USER")) + strlen(path) + 5;
            if ((char)ch == 127) //backspace
            {
                --len;
                if (len < 0)
                    len = 0;
                else
                    callstr[len] = 0;
            }
            else if (ch == 4283163) //я не пьян, это стрелка вверх
            {
                if (histPos == oldhistCount + newhistCount)
                    strcpy(newhist[histPos - oldhistCount], callstr);
                --histPos;
                if (histPos < 0)
                    histPos = 0;
                if (histPos < oldhistCount)
                    strcpy(callstr, oldhist[histPos]);
                else
                    strcpy(callstr, newhist[histPos - oldhistCount]);
                len = strlen(callstr);
                //printf("\nog %d/(%d + %d): %s\n", histPos, oldhistCount, newhistCount, callstr);
            }
            else if (ch == 4348699) //and down
            {
                ++histPos;
                if (histPos > oldhistCount + newhistCount)
                    histPos = oldhistCount + newhistCount;
                if (histPos < oldhistCount)
                    strcpy(callstr, oldhist[histPos]);
                else
                    strcpy(callstr, newhist[histPos - oldhistCount]);
                len = strlen(callstr);
                //printf("\nog %d/(%d + %d): %s\n", histPos, oldhistCount, newhistCount, callstr);
            }
            else if (len < maxCallLen && (char)ch >= ' ')
            {
                i = 0;
                //printf("%c", ch);
                //fflush(stdout);
                while (i < 4) //for Unicode multichar coding (now it is not availiable)
                {
                    char nch = *((char*)(&ch) + i);
                    if ((nch >= ' ' && nch < 127) || (nch < 0))
                    {
                        callstr[len] = nch;
                        ++len;
                    }
                    ++i;
                }
                callstr[len] = 0;

            }
            //else
               // printf("this: %d\n", ch);
            printf("\r");
            if (oldlen > termWidth)
                printf("\033[%dF", oldlen/termWidth);
            for (i = 0; i < oldlen; ++i)
                printf(" ");
            if (oldlen > termWidth)
                printf("\033[%dF", oldlen/termWidth);
            printf("\r[\033[32m%s\033[0m]:%s> %s", getenv("USER"), path, callstr);
            fflush(stdout);
            ch = mygch();
        }
        callstr[len] = 0;
        printf("\n");
        if (len < 1)
            continue;
        if ((oldhistCount == 0 && newhistCount == 0) ||
                (newhistCount == 0 && strcmp(oldhist[oldhistCount - 1], callstr) != 0) ||
                (newhistCount != 0 && strcmp(newhist[newhistCount - 1], callstr) != 0))
        {
            strcpy(newhist[newhistCount], callstr);
            ++newhistCount;
            if (newhistCount == 500)
            {
                appendHistory(newhist, newhistCount);
                oldhistCount = loadHistory(&oldhist);
                newhistCount = 0;
                histPos = oldhistCount;
            }
        }
        else if (newhistCount > 0)
            newhist[newhistCount][0] = 0;
        histPos = oldhistCount + newhistCount;
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
            appendHistory(newhist, newhistCount);
            for (i = 0; i < 6; ++i)
            {
                printf("\033[%dm*\033[0m", 31 + i);
                fflush(stdout);
            }
            printf("\nGoodbye.\n");
            return 0;
        }
        else if (strcmp(comName, "help") == 0)
        {
            printf("Some helpless information about e-bash:\n");
            printf("internal comands:\n");
            printf("\texit - exit from e-bash:(\n");
            printf("\thelp - you are reading it now\n");
            printf("\tcd <path> - change directory to path\n");
            printf("syntax calls instead of internal comands:\n");
            printf("\t[path/]<programm> [args...] [ | <samecall>] [&]\n");
            printf("\tit is execute path/program (or paths from $PATH)\n");
            printf("\twhere samecall:[path/]<programm> [args...] [|<samecall>]\n");
            printf("\tand | it is conveyer chain, & - background launch\n");
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
            if (outErr("cd", code) == 0)
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
            int outfd = 1;
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
                while(progSepar(*nextProg) != 0)
                    ++nextProg;
                if (*nextProg == '>')
                {
                    char filename[256];
                    sscanf(nextProg, ">%s", filename);
                    outfd = open(filename, O_RDWR | O_CREAT, 0666);
                    break;
                }
                while(*nextProg != 0 && progSepar(*nextProg) == 0 || *nextProg <= ' ')
                    ++nextProg;
            }
            if (run_comand_chain(0, outfd, 2, progCount, progNames, allsargv, &code) != 0)
            {
                printf("I can't find this comand: %s\n", comName);
                printf("You can try \"help\", but I think it will not help you\n");
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
