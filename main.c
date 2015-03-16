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
    int code;
    while (1)
    {
        printf(">>\033[32m%s <%s>\033[0m:\n", getenv("USER"));
        code = scanf("%s", s);

        if (strcmp(s, "cd") == 0)
        {
            scanf("%s", path);
            continue;
        }
        if (code == EOF)
        {
            printf("Goodbye.\n");
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
            sprintf(file_addr, "%s%s", path, s);
            //file_addr = strcat(file_addr, s);
            printf("%s: %s\n", getenv("PATH"), file_addr);
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
