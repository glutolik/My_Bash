#include "app_running.h"

const int IS_FIRST = 1;
const int IS_NOT_FIRST = 0;

int run_application(int d_in, int d_out, int d_err, const char* app_name, char *const argv[], int* returned_code)
{
  //делаем ответвление для запуска
	pid_t exec_child = fork();
	if (exec_child == -1)
	{
		perror("Can't fork to run app");
		return -1;
	}
	if (exec_child == 0)
	{
	  //перенаправляем все потоки
		if ((dup2(d_in, 0) == -1) || (dup2(d_out, 1) == -1) || (dup2(d_err, 2) == -1))
        {
            perror("dup2 error.");
            _exit(EXIT_FAILURE);
        }
  	  //выполняем exec
		if (execvp(app_name, argv) == -1)
		{
			perror("exec");
			_exit(EXIT_FAILURE);
		}
	}
	wait(NULL);
  //получаем код возврата
	printf("All done\n");
}

int bind_two_apps(int first, int input_fd, int output_fd, const char* app_name, char *const argv[])
{
	int write_fd, read_fd;
  //если это не последняя команда в цепочке, то создаем pipe
	if (output_fd == -1)
	{
		printf("pipes created for |\n");
		int pipes[2];
		pipe(pipes);
		write_fd = pipes[1];
		read_fd = pipes[0];
	}
  //иначе нужно писать туда, куда сказано(output_fd)
	else
	{
		write_fd = output_fd;
		read_fd = -1;
	}
	run_application(input_fd, write_fd, 2, app_name, argv, NULL);
  //если это не первая команда в цепочке, то следует закрыть соединяющий входной пайп   
    if (first != IS_FIRST)
    {
    	close(input_fd);
    }
  //если это не последняя команда в цепочке, то следует закрыть выходной пайп 
    if (output_fd == -1)
    {
    	close(write_fd);
    }
    return read_fd;
}

int run_comand_chain(int d_in, int d_out, int d_err, const char* app_name, char *const argv[], int* returned_code)
{
	
}