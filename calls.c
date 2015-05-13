#include "calls.h"

int maxCallLen = 512;

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
int getword(const char* stream, char* output)
{
    int i = 0;
    while((stream[i] >= 'a' && stream[i] <= 'z') ||
          (stream[i] >= 'A' && stream[i] <= 'Z') ||
          (stream[i] >= '0' && stream[i] <= '9'))
    {
        output[i] = stream[i];
        ++i;
    }
    if (i < 1)
        return -1;
    output[i] = 0;
    return 0;
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
        int firstlen = 0;
        int reallen = 0;
        char forbindArg[256];
        while (callstr[i + firstlen] > ' ')
        {
            if (callstr[i + firstlen] == '"' && (i + firstlen == 0 || callstr[i + firstlen - 1] != '\\'))
            {
                ++firstlen;
                while (callstr[i + firstlen] != '"' && progSepar(callstr[i + firstlen] != 0))
                {
                    forbindArg[reallen] = callstr[i + firstlen];
                    ++firstlen;
                    ++reallen;
                }
                if (progSepar(callstr[i + firstlen]) != 0)
                    ++firstlen;
            }
            else
            {
                forbindArg[reallen] = callstr[i + firstlen];
                ++firstlen;
                ++reallen;
            }
        }
        forbindArg[reallen] = 0;
        int j = 0;
        char varenv[256];
        arglen = 0;
        for (j = 0; j < reallen; ++j)
        {
            if (forbindArg[j] == '$' && getword(forbindArg + j + 1, varenv) == 0 && getenv(varenv) != NULL)
            {
                arglen = arglen + strlen(getenv(varenv));
                j = j + strlen(varenv) + 1;
            }
            else
                ++arglen;
        }
        (*output)[argnmb] = (char*)malloc((arglen + 1)*sizeof(char));
        arglen = 0;
        for (j = 0; j < reallen; ++j)
        {
            if (forbindArg[j] == '$' && getword(forbindArg + j + 1, varenv) == 0 && getenv(varenv) != NULL)
            {
                strcpy((*output)[argnmb] + arglen, getenv(varenv));
                arglen = arglen + strlen(getenv(varenv));
                j = j + strlen(varenv);
            }
            else
            {
                (*output)[argnmb][arglen] = forbindArg[j];
                ++arglen;
            }
        }
        (*output)[argnmb][arglen] = 0;
        //printf("ps %d: %s\n", argnmb, (*output)[argnmb]);
        i = i + firstlen;
        while (progSepar(callstr[i]) != 0 && callstr[i] <= ' ')
            ++i;
        ++argnmb;
    }
    return argc;
}

int oneStrCall(const char* callstr, char* path, JobsList* jobs)
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
        printf("\tjobfg <nmb> - change job nmb to foreground\n");
        printf("\tjobbg <nmb> - resume job nmb in background\n");
        printf("\tjobsig <nmb> <signal> - send signal to job process\n");
        printf("\te-bash <scriptfile.ebs> - launch e-bash script\n");
        printf("syntax of other calls:\n");
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
        getcwd(path, 255);
        return 0;
    }
    else if (strcmp(comName, "jobs") == 0)
    {
        show_jobs(jobs);
        return 0;
    }
    else if (strcmp(comName, "jobsig") == 0)
    {
        int nmb;
        int sig;
        sscanf(callstr, "jobsig%d%d", &nmb, &sig);
        return signal_process(jobs, nmb, sig);
    }
    else if (strcmp(comName, "jobbg") == 0)
    {
        int nmb;
        sscanf(callstr, "jobsig%d", &nmb);
        return continue_process(jobs, nmb);
    }
    /*else if (strcmp(comName, "ebs"))
    {
        char** argvr;
        oneProgPars(&argvr, callstr + 4);
        scriptRunner(0, 1, argvr);
    }*/
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

int scriptRunner(char** argv)
{
    char* filename = argv[1];
    int scriptfd = open(filename, O_RDONLY);
    if (scriptfd < 0)
    {
        fprintf(stderr, "Error: script file %s does not exist.\n", filename);
        return -1;
    }
    JobsList* jobs = init_jobs_system(50);
    struct stat st;
    fstat(scriptfd, &st);
    char* script = (char*)mmap(NULL, sizeof(char)*st.st_size, PROT_READ, MAP_SHARED, scriptfd, 0);
    int i = 0;
    char path[256];
    getcwd(path, sizeof(path));
    while (i < st.st_size)
    {
        char callstr[maxCallLen];
        int j = i;
        while (j < st.st_size && script[j] != '\n' && script[j] != 0)
            ++j;
        strncpy(callstr, script + i, j - i);
        callstr[j - i] = 0;
        i = j + 1;
        oneStrCall(callstr, path, jobs);
    }
    close(scriptfd);
    return 0;
}
