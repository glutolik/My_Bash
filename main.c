#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

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
        printf("[\033[32m%s\033[0m]:%s>", getenv("USER"), path);
        code = scanf("%s", s);

        if (strcmp(s, "cd") == 0)
        {
            char newpath[256];
            scanf("%s", newpath);
            if (newpath[0] == '/')
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
            }
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
            if (s[0] == '.')
            {
                strcpy(file_addr, path);
                strcat(file_addr, s + 1);
                printf("#starting: %s...\n", file_addr);
                code = execl(file_addr, file_addr, NULL);
            }
            else
            {
                sprintf(file_addr, "/bin/%s", s);
                //file_addr = strcat(file_addr, s);
                printf("%s: %s...\n", path, file_addr);
                code = execl(file_addr, file_addr, path, NULL);
            }

            if (code == -1)
                printf("ERROR: errno = %d\n", errno);

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
