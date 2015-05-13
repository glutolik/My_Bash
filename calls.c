#include "calls.h"

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

int oneStrCall(const char* callstr, char** path)
{
    int len = strlen(callstr);
    if (len < 1)
        return 0;
    char comName[256];
    int i = 0;
    while (callstr[i] > ' ')
        ++i;
    strncpy(comName, callstr, i);
    comName[i] = 0;
    if (strcmp(comName, "exit") == 0) //и аналогично для всех внутренних команд
    {
        /*int i = 0;
        printf("\n");
        appendHistory(newhist, newhistCount);
        for (i = 0; i < 6; ++i)
        {
            printf("\033[%dm*\033[0m", 31 + i);
            fflush(stdout);
        }
        printf("\nGoodbye.\n");
        return 0;*/
        return -2;
    }
    else if (strcmp(comName, "help") == 0)
    {
        printf("Some helpless information about e-bash:\n");
        printf("internal comands:\n");
        printf("\texit - exit from e-bash:(\n");
        printf("\thelp - you are reading it now\n");
        printf("\tcd <path> - change directory to path\n");
        printf("\tjobs - print list of jobs - child processes\n");
        printf("\tfg <nmb> - change job nmb to foreground\n");
        printf("\tbg <nmb> - resume job nmb in background\n");
        printf("syntax calls instead of internal comands:\n");
        printf("\t[path/]<programm> [args...] ['|'<samecall>] [&] [> output|< input]\n");
        printf("\tit is execute path/program (or paths from $PATH)\n");
        printf("\twhere samecall:[path/]<programm> [args...] [|<samecall>]\n");
        printf("\tand | it is conveyer chain, & - background launch\n");
        return 0;
    }
    else if (strcmp(comName, "cd") == 0)
    {
        if (callstr[2] != ' ')
        {
            printf("use cd as \"cd <path>\" or same\n");
            printf("You can try \"help\", but I think it will not help you\n");
            return -1;
        }
        char newDir[256];
        int i = 2;
        while (callstr[i] <= ' ' && callstr[i] != 0)
            ++i;
        strcpy(newDir, callstr + i);
        int code = chdir(newDir);
        if (outErr("cd", code) == 0)
            getcwd(*path, sizeof(*path));
        return 0;
    }
    else if (strcmp(comName, "jobs") == 0 || strcmp(comName, "bg") == 0 || strcmp(comName, "fg") == 0)
    {
        printf("Error: I do not want to do any job!\n");
        return 0;
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
        int infd = 0;
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
            else if (*nextProg == '<')
            {
                char filename[256];
                sscanf(nextProg, "<%s", filename);
                infd = open(filename, O_RDWR | O_CREAT, 0666);
                break;
            }
            while(*nextProg != 0 && progSepar(*nextProg) == 0 || *nextProg <= ' ')
                ++nextProg;
        }
        if (run_comand_chain(infd, outfd, 2, progCount, progNames, allsargv, &code) != 0)
        {
            printf("I can't find this comand: %s\n", comName);
            printf("You can try \"help\", but I think it will not help you\n");
            return -1;
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
        return code;
    }
}

int scriptRunner(int infd, int outfd, char* name, char** argv)
{
    return -1;
}
