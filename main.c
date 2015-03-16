#include <unistd.h>
#include <stdio.h>
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
    for (i = 0; i < 6; ++i)
    {
        printf("\033[%dm*\033[0m", 31 + i);
    }
    printf("\nHello in e-bash!\n");

    while (1)
    {
        //printf(">> \033[32m%s:\033[0m\n", getenv("USER"));
        printf(">> \033[32mUSER:\033[0m%s\n", path);
        code = scanf("%s", s);

        if (strcmp(s, "cd") == 0)
        {
            char newpath[256];
            scanf("%s", newpath);
            if (newpath[0] == '/')
                strcpy(path, newpath);
            else
                strcat(path, newpath);
            continue;
        }
        if (code == EOF || strcmp(s, "exit") == 0)
        {
            int i = 0;
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
            sprintf(file_addr, "/bin/%s", s);
            //file_addr = strcat(file_addr, s);
            printf("%s: %s\n", path, file_addr);
            int code = execl(file_addr, file_addr, NULL);

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
