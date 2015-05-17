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
#include <signal.h>
#include <termios.h>
#include <pthread.h>
#include "app_running.h"
#include "calls.h"
#include "export.c"

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
	tty.c_lflag &= ~(ICANON | ECHO | ISIG);
	tty.c_cc[VMIN] = 1;
	tcsetattr(0, TCSAFLUSH, &tty);
    int ch;
	if (read(0, &ch, sizeof(int)) < 0)
        ch = EOF;
	tcsetattr(0, TCSANOW, &savetty);
	return ch;
}
int intogch()
{
    struct termios tty, savetty;
    tcgetattr(0, &tty);
	savetty = tty;
	tty.c_lflag &= ~(ICANON | ISIG);
	tty.c_cc[VMIN] = 1;
	tcsetattr(0, TCSAFLUSH, &tty);
    int ch;
	if (read(0, &ch, sizeof(int)) < 0)
        ch = EOF;
	tcsetattr(0, TCSANOW, &savetty);
	return ch;
}
int UnicodeSymWidth(int ch)
{
    int i = 0;
    int len = 0;
    while (i < 4) //for Unicode multichar coding (now it is not availiable)
    {
        char nch = *((char*)(&ch) + i);
        if ((nch >= ' ' && nch < 127) || (nch < 0))
        {
            ++len;
        }
        ++i;
    }
    return len;
}

int lastSig = 0;
void sigcc()
{
    lastSig = 2;
}
void listenerTerm()
{
    lastSig = -1;
    char foreof = 5;
    write(0, &foreof, sizeof(char));
}

extern int maxCallLen;

int loadHistory(char*** oldhist)
{
    char histpath[PATH_MAX];
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
    char histpath[PATH_MAX];
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
    char fullPath[PATH_MAX];
    strcpy(fullPath, path);
    strcat(fullPath, file);
    return access(fullPath, F_OK);
}

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "--version") == 0)
        {
            printf("\nTry e-bash. Tolerante and user-friendly.\n");
            int i;
            for (i = 1; i <= 5; ++i)
                printf("\033[%dm*\033[0m", 31 + i);
            for (i = 5; i >= 0; --i)
                printf("\033[%dm*\033[0m", 31 + i);
            printf("\nOnly speed. Only quality. Only e-bash.\n");
            return 0;
        }
        int code = scriptRunner(argv);
        printf("script terminated with code %d.\n", code);
        return code;
    }
    signal(SIGINT, sigcc);
    char callstr[maxCallLen];
    char file_addr[PATH_MAX];
    char path[PATH_MAX];
    getcwd(path, sizeof(path));
    JobsList* jobs = init_jobs_system(50);
    int code;

    int i = 0;
    printf("\n");
    for (i = 5; i >= 0; --i)
    {
        printf("\033[%dm*\033[0m", 31 + i);
    }
    printf("\033]2;You can try e-bash now!\007", path);
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
    //int outpipe[2];
    //pipe(outpipe);
    int savedStdin = -1;

    pid_t prevActive = 0;
    pid_t nowActive = 0;

    while (1)
    {
        nowActive = get_active_pid(jobs);
        if (prevActive == 0)
            prevActive = nowActive;

        if (nowActive != getpid() && nowActive != prevActive)
        {
            fprintf(stderr, "Good luck!\n");
            /*close(outpipe[0]);
            savedStdin = dup(1);
            dup2(outpipe[1], 1);
            close(outpipe[1]);*/
        }
        else if (nowActive != prevActive)
        {
            fprintf(stderr, "Hello in e-bash again!\n");
            /*dup2(savedStdin, 1);
            close(savedStdin);
            pipe(outpipe);*/
        }
        else if (nowActive == getpid())
        {
            printf("[\033[32m%s\033[0m]:%s> ", getenv("USER"), path);
            fflush(stdout);
            i = 0;
            int len = 0;
            int cur = 0;
            callstr[0] = 0;
            int infolen = strlen(getenv("USER")) + strlen(path) + 5;
            int ch;
            //read(0, &ch, 1);
            ch = mygch();
            while ((char)ch != '\n' && (char)ch != EOF) //reading and mdifying callstr
            {
                int i;
                int oldlen = len;
                int oldcur = cur;
                if ((char)ch == 127) //backspace
                {
                    --len;
                    --cur;
                    if (len < 0 || cur < 0)
                    {
                        len = 0;
                        cur = 0;
                    }
                    else
                    {
                        for (i = cur; i <= len; ++i)
                            callstr[i] = callstr[i + 1];
                    }
                }
                else if (ch == 2117294875) //delete
                {
                    if (cur < len)
                    {
                        for (i = cur; i < len; ++i)
                            callstr[i] = callstr[i + 1];
                        --len;
                    }
                }
                else if (ch == 4283163 || ch == -1220453605) //я не пьян, это стрелка вверх.
                                                            //Два числа - это системонезависимость (почти) - x32 и x64
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
                    cur = len;
                }
                else if (ch == 4348699 || ch == -1220388069) //and down
                {
                    ++histPos;
                    if (histPos > oldhistCount + newhistCount)
                        histPos = oldhistCount + newhistCount;
                    if (histPos < oldhistCount)
                        strcpy(callstr, oldhist[histPos]);
                    else
                        strcpy(callstr, newhist[histPos - oldhistCount]);
                    len = strlen(callstr);
                    cur = len;
                }
                else if (ch == 4479771 || ch == -1220256997) //left
                {
                    if (cur > 0)
                        --cur;
                }
                else if (ch == 4414235 || ch == -1220322533) //right
                {
                    if (cur < len)
                        ++cur;
                }
                else if (ch == 4741915 || ch == 2117163803) //home (fn + left)
                {
                    cur = 0;
                }
                else if (ch == 4610843 || ch == 2117360411) //end (fn + right)
                {
                    cur = len;
                }
                else if ((char)ch == '\t') //tab, ch == 32521 on x64
                {
                    /*i = len - 1;
                    char partdir[PATH_MAX];
                    while (i > 0 && callstr[i] != ' ')
                        --i;
                    strcpy(pathdir, callstr + i);*/
                }
                else if (len < maxCallLen - 4 && (char)ch >= ' ')
                {
                    i = 0;
                    //printf("%c", ch);
                    //fflush(stdout);
                    /*while (i < 4) //for Unicode multichar coding (now it is not availiable)
                    {
                        char nch = *((char*)(&ch) + i);
                        if ((nch >= ' ' && nch < 127) || (nch < 0))
                        {
                            int j = len;
                            for (j = len; j > cur; --j)
                                callstr[j] = callstr[j - 1];
                            callstr[cur] = nch;
                            ++len;
                            ++cur;
                        }
                        ++i;
                    }*/
                    int j = len;
                    for (j = len; j > cur; --j)
                        callstr[j] = callstr[j - 1];
                    callstr[cur] = (char)ch;
                    ++len;
                    ++cur;
                    callstr[len] = 0;
                }
                //else
                    //printf("this: %d\n", ch);
                if (infolen + oldcur >= termWidth)
                    printf("\033[%dA", (infolen + oldcur)/termWidth);
                printf("\r");
                for (i = 0; i < oldlen + infolen; ++i)
                    printf(" ");
                if (oldlen + infolen - 1 >= termWidth)
                    printf("\033[%dA", (oldlen + infolen - 1)/termWidth);
                printf("\r[\033[32m%s\033[0m]:%s> %s", getenv("USER"), path, callstr);
                if (infolen + len - 1 >= termWidth)
                    printf("\033[%dA", (infolen + len - 1)/termWidth);
                if (infolen + cur >= termWidth)
                    printf("\033[%dB", (infolen + cur)/termWidth);
                if ((infolen + len)%termWidth == 0 && cur == len)
                    printf("\r\033[%dC\n", termWidth); //new str!
                printf("\r");
                if ((infolen + cur)%termWidth > 0)
                    printf("\033[%dC", (infolen + cur)%termWidth);
                fflush(stdout);
                //read(0, &ch, 1);
                ch = mygch();
            }
            callstr[len] = 0;
            printf("\n");
            if (code == EOF || strcmp(callstr, "exit") == 0) //и аналогично для всех внутренних команд
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

            oneStrCall(callstr, path, jobs);
        }
        else
        {
            char ch = intogch();
            if (ch == EOF)
            {
                printf("ura\n");
                continue;
            }
            else if (ch == 5) //^E
            {
                fprintf(stderr, "uehueueuuu %d ino %d!\n", get_active_pid(jobs), getpid());
                continue;
            }
            else if (ch == 3) //^C
            {
                kill(get_active_pid(jobs), SIGINT);
                //kill(getpid(), SIGKILL); //suicide
                close(get_active_fd(jobs));
                char sch = '\n';
                continue;
            }
            else if (ch == 4) //^D
            {
                close(get_active_fd(jobs));
                continue;
            }
            else if (ch == 26) //^Z
            {
                stop_process(jobs, pid_to_job_number(jobs, get_active_pid(jobs)));
                process_to_background(jobs, pid_to_job_number(jobs, get_active_pid(jobs)));
                continue;
            }
            else
            {
                //fprintf(stderr, "opya0!\n");
                //if (get_active_pid(jobs) != getpid())
                {
                    //fprintf(stderr, "opya1!\n");
                    write(get_active_fd(jobs), &ch, sizeof(char));
                }
                //else
                //    close(1);
            }
        }
        prevActive = nowActive;
    }
    /*void *tmp = NULL;
    wait(tmp);
    printf("%d\n", a);*/
    return 0;
}
