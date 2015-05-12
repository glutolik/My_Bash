#include "app_running.h"

const int IS_FIRST = 1;
const int IS_NOT_FIRST = 0;

int run_application(int d_in, int d_out, int d_err, const char* app_name, char *const argv[], int* returned_code)
{
	int shm = shm_open("/exec_return", O_RDWR | O_CREAT, 0666);
	if (shm == -1)
    {
    	return -1;
    }
    ftruncate(shm, sizeof(int));
    int* exec_return_code = (int*) mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
    if (exec_return_code == NULL)
    {
    	return -1;
    }
  //делаем ответвление для запуска
	pid_t exec_child = fork();
	if (exec_child == -1)
	{
		return -1;
	}
	if (exec_child == 0)
	{
		*exec_return_code = EXIT_SUCCESS;
	  //перенаправляем все потоки
		if ((dup2(d_in, 0) == -1) || (dup2(d_out, 1) == -1) || (dup2(d_err, 2) == -1))
        {
        	*exec_return_code = EXIT_FAILURE;
			exit(EXIT_FAILURE);
        }
  	  //выполняем exec
		if (execvp(app_name, argv) == -1)
		{
			*exec_return_code = EXIT_FAILURE;
			exit(EXIT_FAILURE);
		}
	}
  //получаем код возврата	
	int exit_status;
	wait(&exit_status);
  //если запуск неудачен
	if (*exec_return_code == EXIT_FAILURE || !WIFEXITED(exit_status))
	{
		return -1;
	}	
	if (returned_code != NULL)
	{
		*returned_code = WEXITSTATUS(exit_status);
	}
	return 0;
}

int bind_two_apps(int first, int input_fd, int output_fd, const char* app_name, char *const argv[])
{
	int write_fd, read_fd;
  //если это не последняя команда в цепочке, то создаем pipe
	if (output_fd == -1)
	{
		int pipes[2];
		if (pipe(pipes) == -1)
		{
			return -1;
		}
		write_fd = pipes[1];
		read_fd = pipes[0];
	}
  //иначе нужно писать туда, куда сказано(output_fd)
	else
	{
		write_fd = output_fd;
		read_fd = -1;
	}
	if (run_application(input_fd, write_fd, 2, app_name, argv, NULL) == -1)
	{
		return -1;
	}
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

int run_comand_chain(int d_in, int d_out, int d_err, int comand_count, 
	const char** apps_names, char** apps_args[], int* returned_code)
{
	int next;
	for(size_t i = 0; i < comand_count; ++i)
	{
	  //если команда последняя, то писать в d_out, иначе в соединяющий пайп	
		int current_out = (i + 1 == comand_count)? d_out : -1;
	  //если команда первая, то читать из d_in	
		if (i == 0)
		{
			next = bind_two_apps(IS_FIRST, d_in, current_out, apps_names[i], apps_args[i]);
		}
	  //иначе из соединительного пайпа	
		else
		{
			next = bind_two_apps(IS_NOT_FIRST, next, current_out, apps_names[i], apps_args[i]);	
		}
	}
}