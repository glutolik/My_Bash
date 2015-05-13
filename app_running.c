#include "app_running.h"

const int IS_FIRST = 1;
const int IS_NOT_FIRST = 0;

const char* PROCESS_STATUS_TITLE[] = {"unknown", "terminated", "stopped", "signaled", "no change", "running"};
//process status
const int PS_UNKNOWN = 0;
const int PS_EXITED = 1;
const int PS_STOPED = 2;
const int PS_SIGNALED = 3;
const int PS_NOTHING = 4;
const int PS_RUNNING = 5;

char* generate_process_title(char *const argv[])
{
	char** arg_ptr = argv;
	size_t total_length = strlen(argv[0]);
	while (*arg_ptr != NULL)
	{
		total_length += 1 + strlen(*arg_ptr);
		++arg_ptr;
	}
	char* comand = (char*) malloc(total_length);
	total_length = 0;
	arg_ptr = argv;
	while (*arg_ptr != NULL)
	{
		//total_length += 1;//strlen(*arg_ptr);
		sprintf(comand + total_length, "%s \n", *arg_ptr);
		total_length += strlen(*arg_ptr) + 1;
		++arg_ptr;
	}
	comand[total_length - 1] = '\0';
	return comand;
}

int run_application(int d_in, int d_out, int d_err, const char* app_name, char *const argv[], 
	int* returned_code, JobsList* jobs)
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
	if (run_application(input_fd, write_fd, 2, app_name, argv, NULL, NULL) == -1)
	{
		return -2;
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
	const char** apps_names, char** apps_args[], int* returned_code, JobsList* jobs)
{
	int next;
	pid_t run_child = fork();
	if (run_child == -1)
	{
		return -1;
	}
	if (run_child == 0)
	{
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
			if (next == -2)
			{
				exit(EXIT_FAILURE);
			}
		}
		
		for (size_t i = 0; i < comand_count; ++i)
		{
			wait(NULL);
		}
		exit(EXIT_SUCCESS);
	}
	else
	{
		if (jobs != NULL)
		{
			char* comand = generate_process_title(apps_args[0]);
			printf("Job added\n");
			add_job(jobs, run_child, comand);
		}
		else
		{
			waitpid(run_child, NULL, 0);
		}
	}
	return 0;
}

JobsList* init_jobs_system(size_t new_size)
{
	JobsList* new_jobs_list = (JobsList*) malloc(sizeof(JobsList));
	new_jobs_list->list_size = new_size;
	new_jobs_list->jobs_count = 0;
	new_jobs_list->jobs_list_ptr = (JobStruct*) malloc(sizeof(JobStruct) * new_size);
	return new_jobs_list;
}

void delete_jobs_system(JobsList* jobs)
{
	if (jobs == NULL)
	{
		return;
	}
	for (size_t i = 0; i < jobs->jobs_count; ++i)
	{
		free(jobs->jobs_list_ptr[i].comand_str);
	}
	free(jobs->jobs_list_ptr);
	free(jobs);
}

void add_job(JobsList* jobs, pid_t new_pid, char* name)
{
	jobs->jobs_list_ptr[jobs->jobs_count].pid = new_pid;
	jobs->jobs_list_ptr[jobs->jobs_count].status = PS_RUNNING;
	jobs->jobs_list_ptr[jobs->jobs_count].comand_str = name;
	jobs->jobs_count++;
}

int update_process_status(JobStruct* job)
{
	//kill(job->pid, SIGSTOP);
	int process_info;
	int wait_code = waitpid(job->pid, &process_info, WNOHANG | WUNTRACED | WCONTINUED);
	if (wait_code == -1)
	{
		return -1;
	}
	if (wait_code == job->pid)
	{
		if (WIFEXITED(process_info))
		{
			job->status = PS_EXITED;
		}
		if (WIFSTOPPED(process_info))
		{
			job->status = PS_STOPED;
		}
		if (WIFSIGNALED(process_info))
		{
			//printf("term sig = %d\n", WTERMSIG(process_info));
			job->status = PS_SIGNALED;
		}
		if (WIFCONTINUED(process_info))
		{
			job->status = PS_RUNNING;
		}
	}
	return 0;
}

void show_jobs(JobsList* jobs)
{
	printf("Jobs(%d)[%p]\n", jobs->jobs_count, jobs);
	for (size_t i = 0; i < jobs->jobs_count; ++i)
	{
		if (update_process_status(&(jobs->jobs_list_ptr[i])) == -1)
		{
			continue;
		}
		printf("[%d] %d %s (%s)\n", i, jobs->jobs_list_ptr[i].pid, 
			PROCESS_STATUS_TITLE[jobs->jobs_list_ptr[i].status], jobs->jobs_list_ptr[i].comand_str);
	}
}

int* create_fake_discriptor(void)
{
	int* new_fd = (int*) malloc(2 * sizeof(int));
	pipe(new_fd);
	return new_fd;
}

void delete_fake_discriptor(int* fd)
{
	close(fd[0]);
	close(fd[1]);
	free(fd);
}

int signal_process(JobsList* jobs, size_t job_number, int signal_to_send)
{
	if (job_number < jobs->jobs_count)
	{
		kill(jobs->jobs_list_ptr[job_number].pid, signal_to_send);
		return 0;
	}
	else
	{
		return -1;
	}
}

int stop_process(JobsList* jobs, size_t job_number)
{
	if (signal_process(jobs, job_number, SIGSTOP) == 0)
	{
		printf("Process stoped: %s\n", jobs->jobs_list_ptr[job_number].comand_str);
		return 0;
	}
	return -1;
}

int continue_process(JobsList* jobs, size_t job_number)
{
	if (signal_process(jobs, job_number, SIGCONT) == 0)
	{
		printf("Process continued: %s\n", jobs->jobs_list_ptr[job_number].comand_str);
		return 0;
	}
	return -1;
}