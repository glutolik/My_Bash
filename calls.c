#include "helps.h"
#include "calls.h"

int maxCallLen = 512;

extern char** environ;

extern const int RUN_BACKGROUND;
extern const int RUN_FOREGROUND;

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
    int st = 0;
    while (stream[st] != 0 &&
        !((stream[st] >= 'a' && stream[st] <= 'z') ||
          (stream[st] >= 'A' && stream[st] <= 'Z') ||
          (stream[st] >= '0' && stream[st] <= '9')))
          ++st;
    if (stream[st] == 0)
        return -1;
    if (st > 0 && stream[st - 1] == '$')
    {
        --st;
        output[0] = '$';
        ++i;
    }
    while((stream[st + i] >= 'a' && stream[st + i] <= 'z') ||
          (stream[st + i] >= 'A' && stream[st + i] <= 'Z') ||
          (stream[st + i] >= '0' && stream[st + i] <= '9'))
    {
        output[i] = stream[st + i];
        ++i;
    }
    if (i < 1)
        return -1;
    output[i] = 0;
    return 0;
}
int progSepar(char ch)
{
    if (ch != '|' && ch != '&' && ch != 0 && ch != '(' && ch != ')' && ch != '>' && ch != '<')
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
        char varenv[PATH_MAX];
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
int parsBrakesBody(char* callstr, int* iflen)
{
    int niflen = 0;
    int start = 0;
    int brsum = 1;
    while (callstr[start] != '(')
        ++start;
    while (callstr[start + niflen] != 0 && brsum != 0)
    {
        ++niflen;
        if (callstr[start + niflen] == '(')
            ++brsum;
        else if (callstr[start + niflen] == ')')
            --brsum;
    }
    if (callstr[start + niflen] == 0)
        return 0;
    ++niflen;
    if (iflen != NULL)
        *iflen = start + niflen;
    //printf("%d\n", parsBrakes(callstr, iflen));
    return parsBrakes(callstr + start, niflen);
}

int parsBrakes(const char* callstr, int len)
{
    int i = 0;
    int brsum = 0;
    int brsummin = 0;
    if (len == -1)
        len = strlen(callstr);
    int beg = 0;
    int end = len - 1;
    while (callstr[beg] == ' ' || callstr[beg] == '(')
    {
        if (callstr[beg] == '(')
            ++brsummin;
        ++beg;
    }
    while (end > beg && (callstr[end] == ' ' || callstr[end] == ')'))
        --end;
    brsum = brsummin;
    for (i = beg; i <= end; ++i)
    {
        if (callstr[i] == '(')
            ++brsum;
        else if (callstr[i] == ')')
            --brsum;
        if (brsum < brsummin)
            brsummin = brsum;
    }
    beg = 0;
    end = len - 1;
    while (brsummin > 0)
    {
        while (callstr[beg] != '(')
            ++beg;
        ++beg;
        while (callstr[end] != ')')
            --end;
        --end;
        --brsummin;
    }
    ++end;
    char priorityOper[] = {'|', '&', '=', '!', '<', '>', '+', '-', '*', '/'};
    int prioritySize = 10;
    int oper = 0;
    for (oper = 0; oper < prioritySize; ++oper)
    {
        brsum = 0;
        for (i = beg; i < end; ++i)
        {
            if (callstr[i] == '(')
                ++brsum;
            else if (callstr[i] == ')')
                --brsum;
            if (brsum == 0 && callstr[i] == priorityOper[oper])
            {
                int first = parsBrakes(callstr + beg, i - beg);
                int second = parsBrakes(callstr + i + 1, end - i - 1);
                //fprintf(stderr, "%d vs %d\n", first, second);
                if (callstr[i] == '&')
                    return first*second;
                else if (callstr[i] == '|')
                    return first + second;
                else if (callstr[i] == '=')
                {
                    if (first == second)
                        return 1;
                    else
                        return 0;
                }
                else if (callstr[i] == '!')
                {
                    if (first != second)
                        return 1;
                    else
                        return 0;
                }
                else if (callstr[i] == '<')
                {
                    if (first < second)
                        return 1;
                    else
                        return 0;
                }
                else if (callstr[i] == '>')
                {
                    if (first > second)
                        return 1;
                    else
                        return 0;
                }
                else if (callstr[i] == '*')
                    return first*second;
                else if (callstr[i] == '/')
                    return first/second;
                else if (callstr[i] == '+')
                    return first + second;
                else if (callstr[i] == '-')
                    return first - second;
            }
        }
    }
    int ret = 0;
    int pos = beg;
    char* oneword = callstr + beg;
    while (callstr[pos] <= ' ')
        ++pos;
    if (callstr[pos] == '$')
    {
        i = pos + 1;
        while (callstr[i] != 0 &&
                ((callstr[i] >= 'a' && callstr[i] <= 'z') ||
                 (callstr[i] >= 'A' && callstr[i] <= 'Z') ||
                 (callstr[i] >= '0' && callstr[i] <= '9')))
            ++i;
        //fprintf(stderr, "oKoKo\n");
        char varname[32];
        strncpy(varname, callstr + pos + 1, i - pos - 1);
        varname[i - pos - 1] = 0;
       /* fprintf(stderr, "%s () all levels ok\n", varname);
        if (getenv(varname) == NULL)
            fprintf(stderr, "BUG!!!!!!\n");
        else
            fprintf(stderr, "NO BUG((\n");*/
        oneword = getenv(varname);
        //fprintf(stderr, "%s () getenv ok\n", callstr);
    }
    if (sscanf(oneword, "%d", &ret) == 1)
        return ret;
    pos = 1;
    i = 0;
    ret - 0;
    while (oneword[i] != ' ' && oneword[i] != ' ')
    {
        ret = ret + oneword[i]*pos;
        pos = pos * 256;
        ++i;
    }
    return ret;
}

char writerargs[PATH_MAX];

static void* envvarWriter(void* frfd)
{
    int fromfd = *((int*)frfd);
    char varname[PATH_MAX];
    strcpy(varname, writerargs);
    char output[PATH_MAX];
    int len = 0;
    output[0] = 0;
    char ch = 0;
    while (read(fromfd, &ch, 1) > 0)
    {
        output[len] = ch;
        ++len;
        output[len] = 0;
    }
    setenv(varname, output, 1);
    pthread_exit(NULL);
}

int oneStrCall(const char* callstr, char* path, JobsList* jobs, int infdFrom)
{
    int i = 0;
    while (callstr[i] <= ' ')
        ++callstr;
    int len = strlen(callstr);
    if (len < 1)
        return 0;
    char comName[256];
    while (callstr[i] > ' ')
        ++i;
    strncpy(comName, callstr, i);
    comName[i] = 0;
    if (strcmp(comName, "exit") == 0) //и аналогично для всех внутренних команд
    {
        return -2;
    }
    else if (strcmp(comName, "help") == 0)
    {
        char func[32];
        if (sscanf(callstr, "help%s", func) != 1)
            strcpy(func, "all");
        return printHelp(func);
    }
    else if (strcmp(comName, "cd") == 0)
    {
        if (callstr[2] != ' ')
        {
            printf("use cd as \"cd <path>\" or same\n");
            printf("You can try \"help\", but I think it will not help you\n");
            return -1;
        }
        char newDir[PATH_MAX];
        int i = 2;
        while (callstr[i] <= ' ' && callstr[i] != 0)
            ++i;
        strcpy(newDir, callstr + i);
        int code = chdir(newDir);
        if (code == 0)
        {
            getcwd(path, PATH_MAX);
            printf("\033]2;e-bash in %s\007", path);
            fflush(stdout);
        }
        return code;
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
        sscanf(callstr, "jobbg%d", &nmb);
        return continue_process(jobs, nmb);
    }
    else if (strcmp(comName, "jobfg") == 0)
    {
        int nmb;
        sscanf(callstr, "jobfg%d", &nmb);
        return process_to_foreground(jobs, nmb);
    }
    else if (strcmp(comName, "jobstop") == 0)
    {
        int nmb;
        sscanf(callstr, "jobstop%d", &nmb);
        return stop_process(jobs, nmb);
    }
    else if (strcmp(comName, "envvar") == 0)
    {
        char varname[32];
        char value[PATH_MAX];
        if (getword(callstr + 6, varname) != -1 && getword(callstr + 7 + strlen(varname) + 1, value) != -1)
        {
            int i = 6 + strlen(varname) + 1;
            while (callstr[i] == ' ' || callstr[i] == '=')
                ++i;
            if (callstr[i] == '(')
            {
                sprintf(value, "%d", parsBrakesBody(callstr + i - 1, NULL));
            }
            if (value[0] == '$')
            {
                char* repl = getenv(value + 1);
                if (repl != NULL)
                    strcpy(value, repl);
            }
            return setenv(varname, value, 1);
        }
        int i = 0;
        while (environ[i] != NULL)
        {
            printf("%s\n", environ[i]);
            ++i;
        }
        return 0;
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
        int infd = infdFrom;
        int outfd = 1;
        int outfdClosing = 0;
        int BGflag = RUN_FOREGROUND;
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
                char filename[PATH_MAX];
                sscanf(nextProg, ">%s", filename);
                if (filename[0] == '$')
                {
                    int outpipe[2];
                    pipe(outpipe);
                    outfd = outpipe[1];
                    strcpy(writerargs, filename + 1);
                    pthread_t writer;
                    pthread_create(&writer, NULL, envvarWriter, (void*)(&outpipe[0]));
                    outfdClosing = 1;
                }
                else if (filename[0] != '>')
                    outfd = open(filename, O_RDWR | O_CREAT, 0666);
                else
                {
                    sscanf(nextProg, ">>%s", filename);
                    outfd = open(filename, O_WRONLY | O_APPEND | O_CREAT, 0666);
                }
            }
            else if (*nextProg == '<')
            {
                char filename[PATH_MAX];
                sscanf(nextProg, "<%s", filename);
                if (filename[0] == '$')
                {
                    filename[0] = '/';
                    infd = shm_open(filename, O_RDWR | O_CREAT, 0666);
                    char* value = getenv(filename + 1);
                    ftruncate(infd, strlen(value) + 1);
                    char* shmvalue = (char*)mmap(NULL, strlen(value) + 1, PROT_READ | PROT_WRITE, MAP_SHARED, infd, 0);
                    strcpy(shmvalue, value);
                    munmap(shmvalue, strlen(value));
                    close(infd);
                    infd = shm_open(filename, O_RDONLY, 0666);
                }
                else
                    infd = open(filename, O_RDWR | O_CREAT, 0666);
            }
            else if (*nextProg == '&')
                BGflag = RUN_BACKGROUND;
            while(*nextProg != 0 && (progSepar(*nextProg) == 0 || *nextProg <= ' '))
                ++nextProg;
        }
        if (run_comand_chain(infd, outfd, 2, progCount, progNames, allsargv, &code, jobs, BGflag) != 0)
        {
            printf("I can't find this comand: %s\n", comName);
            printf("You can try \"help\", but I think it will not help you\n");
            return -1;
        }
        if (outfdClosing != 0)
        {
            close(outfd);
            outfdClosing = 0;
        }
        for (i = 0; i < progCount; ++i)
        {
            int j = 0;
            while(allsargv[i][j] != NULL)
            {
                //fprintf(stderr, "arg # %d: %s\n", j, allsargv[i][j]);
                free(allsargv[i][j]);
                //fprintf(stderr, "arg # %d ok\n", j);
                ++j;
            }
            free(progNames[i]);
            free(allsargv[i]);
        }
        return code;
    }
}

int scriptBlockRunner(const char* script, int size, JobsList* jobs, char* path)
{
    int i = 0;
    while (i < size)
    {
        char callstr[maxCallLen];
        char comName[32];
        while (i < size && script[i] <= ' ')
            ++i;
        int j = i;
        while (j < size && script[j] != '\n' && script[j] != 0 && (j >= size - 1 || script[j] != '/' || script[j + 1] != '/'))
            ++j;
        if (j >= size)
            return 0;
        strncpy(callstr, script + i, j - i);
        callstr[j - i] = 0;
        getword(callstr, comName);
        if (strcmp(comName, "break") == 0)
        {
            int retcode = 0;
            sscanf(callstr, "break%d", &retcode);
            return retcode;
        }
        if (strcmp(comName, "if") == 0 || strcmp(comName, "while") == 0)
        {
            int iflen = i;
            int beg = i + strlen(comName);
            while (beg < size && script[beg] != '{')
                ++beg;
            ++beg;
            int end = beg;
            int brsum = 1;
            while (script[end] != 0 && brsum != 0)
            {
                ++end;
                if (script[end] == '{')
                    ++brsum;
                else if (script[end] == '}')
                    --brsum;
            }
            //printf("%d ... %d (%d): %c\n", beg, end, size, script[beg]);
            if (end >= size)
                return -1;
            while (parsBrakesBody(callstr + strlen(comName), &iflen) > 0)
            {
                int code = scriptBlockRunner(script + beg, end - beg, jobs, path);
                if (code != 0)
                    return code;
                if (strcmp(comName, "if") == 0)
                    break;
            }
            i = end;
            while (script[i] != '\n')
                ++i;
            ++i;
        }
        else
        {
            oneStrCall(callstr, path, jobs, 0);
            //show_jobs(jobs);
            wait_while_running(jobs);
            if (j < size - 1 && script[j] == '/' && script[j + 1] == '/')
                while (j < size && script[j] != '\n' && script[j] != 0)
                    ++j;
            i = j + 1;
        }
    }
    return 0;
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
    JobsList* jobs = init_jobs_system(5000); //for script it should be bigger
    struct stat st;
    fstat(scriptfd, &st);
    char* script = (char*)mmap(NULL, sizeof(char)*st.st_size, PROT_READ, MAP_SHARED, scriptfd, 0);
    int i = 1;
    while (argv[i] != NULL)
    {
        char argstr[10];
        sprintf(argstr, "arg%d", i - 1);
        setenv(argstr, argv[i], 1);
        ++i;
    }
    char argcstr[10];
    sprintf(argcstr, "%d", i - 1);
    setenv("argc", argcstr);
    char path[PATH_MAX];
    getcwd(path, sizeof(path));
    int code = scriptBlockRunner(script, st.st_size, jobs, path);
    close(scriptfd);
    delete_jobs_system(jobs);
    return code;
}
