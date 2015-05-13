#ifndef APP_RUNNING
#define APP_RUNNING

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <signal.h>

extern const int IS_FIRST;
extern const int IS_NOT_FIRST;

typedef struct 
{
	pid_t pid;
	int status;
	char* comand_str;
} JobStruct;

typedef struct
{
	JobStruct* jobs_list_ptr;
	size_t list_size;
	size_t jobs_count;
} JobsList;

int run_application(int d_in, int d_out, int d_err, const char* app_name, char *const argv[], 
	int* returned_code, JobsList* jobs);
int bind_two_apps(int first, int input_fd, int output_fd, const char* app_name, char *const argv[]);

int run_comand_chain(int d_in, int d_out, int d_err, int comand_count, 
	const char** apps_names, char** apps_args[], int* returned_code);

JobsList* init_jobs_system(size_t new_size);
void delete_jobs_system(JobsList* jobs);
void add_job(JobsList* jobs, pid_t new_pid, char* name);
void show_jobs(JobsList* jobs);
int* create_fake_discriptor(void);
void delete_fake_discriptor(int* fd);
int signal_process(JobsList* jobs, size_t job_number, int signal_to_send);
int stop_process(JobsList* jobs, size_t job_number);
int continue_process(JobsList* jobs, size_t job_number);
#endif