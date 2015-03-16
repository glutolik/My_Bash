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
int pathAcc(const char* path, const char* file)
{
    char fullPath[256];
    strcpy(fullPath, path);
    strcat(fullPath, file);
    return access(fullPath, F_OK);
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
        memset(s, sizeof(s), 0);
        gets(s);
        //scanf("%s", s);
        char comName[256];
        int endCom = 0;
        while (s[endCom] > ' ')
            ++endCom;
        strncpy(comName, s, endCom);
        comName[endCom] = 0;
        if (code == EOF || strcmp(comName, "exit") == 0) //и аналогично для всех внутренних команд
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
        else if (strcmp(comName, "cd") == 0)
        {
            if (s[endCom] != ' ')
            {
                printf("use cd as \"cd directory\" or same\n");
                printf("You can see \"help\", which hasn't realized yet\n");
                continue;
            }
            char newDir[256];
            while (s[endCom] <= ' ' && s[endCom] != 0)
                ++endCom;
            strcpy(newDir, s + endCom);
            int code = chdir(newDir);
            if (outErr(code) == 0)
                getcwd(path, sizeof(path));
            continue;
        }
        //printf("%s isn't a internal comand\n", comName);

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
            char comName[256];
            strcpy(comName, nextArg);

            i = 1;
            while (nextArg != NULL && i < argc)
            {
                nextArg = strtok(NULL, " ");
                strcpy(argv[i], nextArg);
                ++i;
            }

            if (s[0] == '.' && pathAcc("", comName) == 0)
            {
                strcpy(file_addr, path);
                strcat(file_addr, comName + 1);
                printf("#starting: %s...\n", file_addr);
            }
            else if (pathAcc("/bin/", comName) == 0) //и аналогично для всех директорий из $PATH
            {
                sprintf(file_addr, "/bin/%s", comName);
                //strcat(file_addr, s);
                printf("%s: %s...\n", path, file_addr);
            }
            else
            {
                printf("I can't find this comand: %s\n", comName);
                printf("You can see \"help\", which hasn't realized yet\n");
                return 1;
            }

            strcpy(argv[0], file_addr);
            char** argvr = (char**)malloc(argc*sizeof(char*));
            for (i = 0; i < argc; ++i)
            {
                argvr[i] = (char*)malloc(strlen(argv[i])*sizeof(char));
                strncpy(argvr[i], argv[i], strlen(argv[i]));
            }
            //code = execl(file_addr, file_addr, NULL);
            code = execv(file_addr, argvr);

            free(argvr);

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
