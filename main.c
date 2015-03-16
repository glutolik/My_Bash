#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>

int outErr(int n)
{
    if (n != 0)
        fprintf(stderr, "ERROR: errno = %d\n", errno);
    return n;
}
int main(int argc, char **argv)
{
    char s[256];
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
        gets(s);
        //scanf("%s", s);

        if (strcmp(s, "cd") == 0)
        {
            char newpath[256];
            scanf("%s", newpath);
            /*if (newpath[0] == '/')
                strcpy(path, newpath);
            else if (strcmp(newpath, "..") == 0)
            {
                int len = strlen(path);
                int newlen = len - 1;
                while (newlen > 0 && path[newlen] != '/')
                    --newlen;
                for (i = newlen; i <= len; ++i)
                        path[i] = 0;
                path[0] = '/';
            }
            else
            {
                path[strlen(path)] = '/';
                strcat(path, newpath);
            }*/
            int code = chdir(newpath);
            if (outErr(code) == 0)
                getcwd(path, sizeof(path));
            continue;
        }
        if (code == EOF || strcmp(s, "exit") == 0)
        {
            int i = 0;
            printf("\n");
            for (i = 0; i < 6; ++i)
            {
                printf("\033[%dm*\033[0m", 31 + i);
            }
            printf("\nGoodbye.\n");
            return 0;
        }

        pid_t child = fork();

        if (child == -1)
        {
            perror("Can't fork.");
            return 1;
        }

        if (child == 0)
        {
            int code;
            int i;
            int len = strlen(s);
            int argc = 1;
            for (i = 0; i < len; ++i)
                if (s[i] == ' ')
                    ++argc;
            char argv[argc][256];
            char *nextArg = strtok(s, " ");
            if (s[0] == '.')
            {
                strcpy(file_addr, path);
                strcat(file_addr, nextArg + 1);
                strcpy(argv[0], file_addr);
                printf("#starting: %s...\n", file_addr);
            }
            else
            {
                sprintf(file_addr, "/bin/%s", nextArg);
                //strcat(file_addr, s);
                strcpy(argv[0], file_addr);
                printf("%s: %s...\n", path, file_addr);
            }

            i = 1;
            while (nextArg != NULL && i < argc)
            {
                nextArg = strtok(NULL, " ");
                strcpy(argv[i], nextArg);
                ++i;
            }
            char** argvr = (char**)malloc(argc*sizeof(char*));
            for (i = 0; i < argc; ++i)
            {
                argvr[i] = (char*)malloc(strlen(argv[i])*sizeof(char));
                strncpy(argvr[i], argv[i], strlen(argv[i]));
            }
            //code = execl(file_addr, file_addr, NULL);
            code = execv(file_addr, argvr);

            outErr(code);

            return 0;
        }
        void *status = NULL;
        wait(status);
    }
    /*void *tmp = NULL;
    wait(tmp);
    printf("%d\n", a);*/
    return 0;
}
