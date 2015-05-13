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
#include "calls.h"

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
        int cur = 0;
        callstr[0] = 0;
        int ch = mygch();
        while ((char)ch != '\n' && (char)ch != EOF) //reading callstr
        {
            int i;
            int oldlen = len + strlen(getenv("USER")) + strlen(path) + 5;
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
            else if (ch == 2117294875)
            {
                if (cur < len)
                {
                    for (i = cur; i < len; ++i)
                        callstr[i] = callstr[i + 1];
                    --len;
                }
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
                cur = len;
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
                cur = len;
            }
            else if (ch == 4479771) //left
            {
                if (cur > 0)
                    --cur;
            }
            else if (ch == 4414235) //right
            {
                if (cur < len)
                    ++cur;
            }
            else if (ch == 4741915) //home (fn + left)
            {
                cur = 0;
            }
            else if (ch == 4610843) //end (fn + right)
            {
                cur = len;
            }
            else if ((char)ch == '\t') //tab, ch == 32521
            {
            }
            else if (len < maxCallLen - 4 && ((char)ch >= ' ' || (char)ch < 0))
            {
                i = 0;
                //printf("%c", ch);
                //fflush(stdout);
                while (i < 4) //for Unicode multichar coding (now it is not availiable)
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
                }
                callstr[len] = 0;
            }
            //else
                //printf("this: %d\n", ch);
            printf("\r");
            if (oldlen > termWidth)
                printf("\033[%dF", oldlen/termWidth);
            for (i = 0; i < oldlen; ++i)
                printf(" ");
            if (oldlen > termWidth)
                printf("\033[%dF", oldlen/termWidth);
            printf("\r[\033[32m%s\033[0m]:%s> %s", getenv("USER"), path, callstr);
            oldlen = strlen(getenv("USER")) + strlen(path) + 5;
            if (oldlen + len > termWidth)
                printf("\033[%dF", (oldlen + len)/termWidth);
            if (oldlen + cur > termWidth)
                printf("\033[%dE", (oldlen + cur)/termWidth);
            printf("\r\033[%dC", (oldlen + cur)%termWidth);
            fflush(stdout);
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

        oneStrCall(callstr, &path);
    }
    /*void *tmp = NULL;
    wait(tmp);
    printf("%d\n", a);*/
    return 0;
}
