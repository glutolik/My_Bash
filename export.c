#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>


//to print all memebers of __environ use flag "-p"
int exportp(){
	int i=0;

    	while (__environ[i] != NULL) {
        	printf("%s\n",__environ[i]);
       		i++;
	};
//	printf("%s\n", __environ[i-1]);
    return 0;
}

//to delete from __environ by name use flag "-n"
int exportn(const char* name){
	return  unsetenv(name);
}

//this function is used to add var to __environ. export([name = value])
int export(char* var){
	size_t i, j;
	int len = strlen(var);
	char* buf1 = (char*)malloc(sizeof(char)*len);
	char* buf2 = (char*)malloc(sizeof(char)*len);

	if(var[0] == '='){
		printf("bash: export: '%s': wrong identifier\n", var);
		return -1;
	}
	if(var[len-1] == '='){
		strncpy(buf1, var, len -1);
		size_t len1 = strlen(buf1);
		for(j = 0; j < len1; j++){
			if(buf1[j] == '='){
				printf("bash: export: '%s': wrong identifier\n", var);
				return -1;
			}
		}
		setenv(buf1, "", 1);
		printf("ENVVAR set: %s = ''''\n", buf1);
		return 0;
	}
	for(i = 1; i < len-1; i++){
		if(var[i] == '='){
			strncpy(buf1, var, i);
			strncpy(buf2, var + i + 1, len - i - 1);
			size_t len1 = strlen(buf1);
			size_t len2 = strlen(buf2);
			for(j = 0; j < len1; j++){
				if(buf1[j] == '='){
					printf("bash: export: '%s': wrong identifier\n", var);
					return -1;
				}
			}
			for(j = 0; j < len2; j++){
				if(buf2[j] == '='){
					printf("bash: export: '%s': wrong identifier\n", var);
					return -1;
				}
			}
			printf("%s\n%s\n", buf1, buf2);
			setenv(buf1, buf2, 1);
			printf("ENVVAR set: %s = '%s'\n", buf1, buf2);
			free(buf1);
			free(buf2);
			return 0;
		}
	}

	return -1;
}

//int main(){
//	char* var = (char*)malloc(sizeof(char)*256);
//	scanf("%s", var);
//	printf("\n\n\n===================\n");
//	export(var);
//	exportp();
//	exportn("NAMEEE");
//	printf("\n\n\n===================\n");
//	exportp();
//	free(var);
//	return 0;
//
//
//}
