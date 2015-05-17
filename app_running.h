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

extern const int RUN_BACKGROUND;
extern const int RUN_FOREGROUND;

typedef struct 
{
	pid_t pid;
	int status;
	int fg_flag;
	int current_input;
	int real_input;
	int* fake_fd;
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

int run_comand_chain(int d_in, int d_out, int d_err, int pipe_write, int comand_count, 
	const char** apps_names, char** apps_args[], int* returned_code, JobsList* jobs, int bg_flag);

JobsList* init_jobs_system(size_t new_size);
void delete_jobs_system(JobsList* jobs);
void add_job(JobsList* jobs, pid_t pid, char* name, int* fake_fd, int current_in, int real_in, int fg_flag);
void show_jobs(JobsList* jobs);
int* create_fake_discriptor(void);
void delete_fake_discriptor(int* fd);
int signal_process(JobsList* jobs, size_t job_number, int signal_to_send);
int stop_process(JobsList* jobs, size_t job_number);
int continue_process(JobsList* jobs, size_t job_number);
int process_to_foreground(JobsList* jobs, size_t job_number);
int process_to_background(JobsList* jobs, size_t job_number);
pid_t get_active_pid(JobsList* jobs);
ssize_t pid_to_job_number(JobsList* jobs, pid_t pid);
#endif