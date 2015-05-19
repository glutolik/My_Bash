#include "app_running.h"

const int IS_FIRST = 1;
const int IS_NOT_FIRST = 0;

const char* PROCESS_STATUS_TITLE[] = {"unknown", "terminated", "stopped", "signaled", "no change", "running"};
const int PROCESS_STATUS_COLORS[] = {36, 37, 33, 35, 0, 34};

const int PS_UNKNOWN = 0;
const int PS_EXITED = 1;
const int PS_STOPED = 2;
const int PS_SIGNALED = 3;
const int PS_NOTHING = 4;
const int PS_RUNNING = 5;

const int FG_IS_ACTIVE = 1;
const int FG_IS_NOT_ACTIVE = 0;
const int FG_IS_DEAD = -1;

const int RUN_BACKGROUND = 0;
const int RUN_FOREGROUND = 1;

int run_application(int d_in, int d_out, int d_err, const char* app_name, char *const argv[], 
	int* returned_code, JobsList* jobs)
{
  //делаем ответвление для запуска
	pid_t exec_child = fork();
	if (exec_child == -1)
	{
		return -1;
	}
	if (exec_child == 0)
	{
	  //перенаправляем все потоки
		dup2(d_in, 0);
		dup2(d_out, 1);
		dup2(d_err, 2);
  	  //выполняем exec
		if (execvp(app_name, argv) == -1)
		{
			fprintf(stderr, "I can't find this comand: %s\n", app_name);
            fprintf(stderr, "You can try \"help\", but I think it will not help you\n");
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

char* generate_process_title(size_t argc, char** argv[])
{
	size_t total_length = 0;
	for (size_t i = 0; i < argc; ++i)
	{
		char** arg_ptr = argv[i];
		while (*arg_ptr != NULL)
		{
			total_length += 1 + strlen(*arg_ptr);
			++arg_ptr;
		}
		if (i + 1 != argc)
		{
			total_length += 2;
		}
	}
	//printf("total name length = %d\n", total_length);
	char* comand = (char*) malloc(total_length);
	size_t offset = 0;
	for (size_t i = 0; i < argc; ++i)
	{
		char** arg_ptr = argv[i];
		while (*arg_ptr != NULL)
		{
			sprintf(comand + offset, "%s ", *arg_ptr);
			offset += strlen(*arg_ptr) + 1;
			++arg_ptr;
		}
		if (i + 1 != argc)
		{
			sprintf(comand + offset, "| ");
			offset += 2;
		}
	}
	comand[total_length - 1] = '\0';
	return comand;
}

int run_comand_chain(int d_in, int d_out, int d_err, int comand_count, 
	const char** apps_names, char** apps_args[], int* returned_code, JobsList* jobs, int bg_flag)
{
  //создаем пайп для общения c bash'ем	
	int* input_pipe = (int*) malloc(sizeof(int) * 2);
	pipe(input_pipe);
  //если читать нужно не из stdin, перенаправим чтение процесса в нужное место
    if (d_in != -1)	
    {
    	dup2(d_in, input_pipe[0]);
    }
    int next;
	pid_t run_child = fork();
	if (run_child == -1)
	{
		return -1;
	}
	if (run_child == 0)
	{
		close(input_pipe[1]);
		for(size_t i = 0; i < comand_count; ++i)
		{
		  //если команда последняя, то писать в d_out, иначе в соединяющий пайп	
			int current_out = (i + 1 == comand_count)? d_out : -1;
		  //если команда первая, то читать из "входного потока"
			if (i == 0)
			{
				next = bind_two_apps(IS_FIRST, input_pipe[0], current_out, apps_names[i], apps_args[i]);
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
		int exit_code = 0;
		for (size_t i = 0; i < comand_count; ++i)
		{
			int exit_info;
			wait(&exit_info);
			if (WIFEXITED(exit_info) && WEXITSTATUS(exit_info) != 0)
			{
				exit_code = WEXITSTATUS(exit_info);
			}
		}
		exit(exit_code);
	}
	else
	{
		close(input_pipe[0]);
		char* comand = generate_process_title(comand_count, apps_args);
		if (bg_flag == RUN_BACKGROUND)
		{
			add_job(jobs, run_child, comand, input_pipe, FG_IS_NOT_ACTIVE);
		}
		else
		{
			add_job(jobs, run_child, comand, input_pipe, FG_IS_ACTIVE);
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
		close(jobs->jobs_list_ptr[i].fd[0]);
		close(jobs->jobs_list_ptr[i].fd[1]);
		free(jobs->jobs_list_ptr[i].fd);
	}
	free(jobs->jobs_list_ptr);
	free(jobs);
}

void add_job(JobsList* jobs, pid_t pid, char* name, int* new_fd, int fg_flag)
{
	if (jobs->jobs_count >= jobs->list_size)
	{
		jobs->list_size *= 2;
		jobs->jobs_list_ptr = realloc(jobs->jobs_list_ptr, jobs->list_size * sizeof(JobStruct));
	}
	jobs->jobs_list_ptr[jobs->jobs_count].pid = pid;
	jobs->jobs_list_ptr[jobs->jobs_count].status = PS_RUNNING;
	jobs->jobs_list_ptr[jobs->jobs_count].comand_str = name;
	jobs->jobs_list_ptr[jobs->jobs_count].fd = new_fd;
	jobs->jobs_list_ptr[jobs->jobs_count].fg_flag = fg_flag;
	jobs->jobs_count++;
}

int analise_wait_status(JobsList* jobs, size_t job_number, int process_info, int wait_code)
{
  //-1 если такого процесса не существует	
	if (wait_code == -1)
	{
		if (if_process_exist(jobs, job_number) == 0)
		{
			jobs->jobs_list_ptr[job_number].status = PS_UNKNOWN;
			jobs->jobs_list_ptr[job_number].fg_flag = FG_IS_DEAD;
		}
		return -1;
	}
  //pid если есть изменения в сотоянии процесса	
	if (wait_code == jobs->jobs_list_ptr[job_number].pid)
	{
		if (WIFEXITED(process_info))
		{
			jobs->jobs_list_ptr[job_number].term_info = WEXITSTATUS(process_info);
			jobs->jobs_list_ptr[job_number].status = PS_EXITED;
			jobs->jobs_list_ptr[job_number].fg_flag = FG_IS_DEAD;
		}
		if (WIFSTOPPED(process_info))
		{
			jobs->jobs_list_ptr[job_number].status = PS_STOPED;
			process_to_background(jobs, job_number);
		}
		if (WIFSIGNALED(process_info))
		{
			jobs->jobs_list_ptr[job_number].term_info = WTERMSIG(process_info);
			jobs->jobs_list_ptr[job_number].status = PS_SIGNALED;
			jobs->jobs_list_ptr[job_number].fg_flag = FG_IS_DEAD;
		}
		if (WIFCONTINUED(process_info))
		{
			jobs->jobs_list_ptr[job_number].status = PS_RUNNING;
		}
	}	
	return 0;
}

int update_process_status(JobsList* jobs, size_t job_number)
{
	int process_info;
	int wait_code = waitpid(jobs->jobs_list_ptr[job_number].pid, &process_info, WNOHANG | WUNTRACED | WCONTINUED);
    return analise_wait_status(jobs, job_number, process_info, wait_code);
}

void show_jobs(JobsList* jobs)
{
	for (size_t i = 0; i < jobs->jobs_count; ++i)
	{
		update_process_status(jobs, i);
		if (jobs->jobs_list_ptr[i].fg_flag == FG_IS_DEAD && jobs->jobs_list_ptr[i].term_info == 0)
		{
			continue;
		}
		printf("\033[%dm", PROCESS_STATUS_COLORS[jobs->jobs_list_ptr[i].status]);
		printf("[%d]", i);
		if (jobs->jobs_list_ptr[i].fg_flag != FG_IS_DEAD)
		{
			printf("*");
		}
		else
		{
			printf("-");
		}
		printf(" %d %s ", jobs->jobs_list_ptr[i].pid, PROCESS_STATUS_TITLE[jobs->jobs_list_ptr[i].status]);
		if (jobs->jobs_list_ptr[i].status == PS_EXITED)
		{
			printf("with code %d ", jobs->jobs_list_ptr[i].term_info);
		}
		if (jobs->jobs_list_ptr[i].status == PS_SIGNALED)
		{
			printf("by signal %d ", jobs->jobs_list_ptr[i].term_info);
		}
		printf("(%s)\n", jobs->jobs_list_ptr[i].comand_str);	
		printf("\033[0m");
		fflush(stdout);
	}
}

int if_process_exist(JobsList* jobs, size_t job_number)
{
	if (job_number < jobs->jobs_count && jobs->jobs_list_ptr[job_number].fg_flag != FG_IS_DEAD)
	{
		return 0;
	}
	else
	{
		return -1;
	}	
}

int signal_process(JobsList* jobs, size_t job_number, int signal_to_send)
{
	if (if_process_exist(jobs, job_number) == 0)
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
		process_to_background(jobs, job_number);
		fprintf(stderr, "Process stoped: %s\n", jobs->jobs_list_ptr[job_number].comand_str);
		return 0;
	}
	return -1;
}

int continue_process(JobsList* jobs, size_t job_number)
{
	if (signal_process(jobs, job_number, SIGCONT) == 0)
	{
		fprintf(stderr, "Process continued: %s\n", jobs->jobs_list_ptr[job_number].comand_str);
		return 0;
	}
	return -1;
}

int process_to_foreground(JobsList* jobs, size_t job_number)
{
	if (if_process_exist(jobs, job_number) == 0)
	{
		jobs->jobs_list_ptr[job_number].fg_flag = FG_IS_ACTIVE;	
		continue_process(jobs, job_number);
		return 0;
	}
    return -1;
}

int process_to_background(JobsList* jobs, size_t job_number)
{
	if (if_process_exist(jobs, job_number) == 0)
	{
		jobs->jobs_list_ptr[job_number].fg_flag = FG_IS_NOT_ACTIVE;	
		return 0;
	}
	return -1;
}

JobStruct* get_active_job(JobsList* jobs)
{
	JobStruct* result = NULL;
	for (size_t i = 0; i < jobs->jobs_count; ++i)
	{
		update_process_status(jobs, i);
		if (jobs->jobs_list_ptr[i].fg_flag == FG_IS_ACTIVE)
		{
			result = &(jobs->jobs_list_ptr[i]);
			break;
		}
	}
	return result;
}

pid_t get_active_pid(JobsList* jobs)
{
	JobStruct* active_job_ptr = get_active_job(jobs);
	if (active_job_ptr == NULL)
	{
		return getpid();
	}
	else
	{
		return active_job_ptr->pid;
	}
}

int get_active_fd(JobsList* jobs)
{
	JobStruct* active_job_ptr = get_active_job(jobs);
	if (active_job_ptr == NULL)
	{
		return -1;
	}
	else
	{
		return active_job_ptr->fd[1];
	}
}

ssize_t pid_to_job_number(JobsList* jobs, pid_t pid)
{
	for (size_t i = 0; i < jobs->jobs_count; ++i)
	{
		if (jobs->jobs_list_ptr[i].pid == pid)
		{
			return i;
		}
	}
	return -1;
}

int wait_while_running(JobsList* jobs)
{
	JobStruct* active_job = get_active_job(jobs);
	if (active_job == NULL)
	{
		return 0;
	}
	int process_info;
	int wait_code = waitpid(active_job->pid, &process_info, WUNTRACED);	
	size_t active_job_number = active_job - jobs->jobs_list_ptr;
	analise_wait_status(jobs, active_job_number, process_info, wait_code);
}