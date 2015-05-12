#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>

extern const int IS_FIRST;
extern const int IS_NOT_FIRST;

int run_application(int d_in, int d_out, int d_err, const char* app_name, char *const argv[], int* returned_code);
int bind_two_apps(int first, int input_fd, int output_fd, const char* app_name, char *const argv[]);

int run_comand_chain(int d_in, int d_out, int d_err, int comand_count, 
	const char** apps_names, char** apps_args[], int* returned_code);