#include "helps.h"

int printHelp(const char* func)
{
    if (func == NULL || strcmp(func, "all") == 0)
    {
        printf("Some helpless information about e-bash:\n");
        printf("internal comands:\n");
        printf("\texit - exit from e-bash:(\n");
        printf("\thelp - you are reading it now\n");
        printf("\tcd <path> - change directory to path\n");
        printf("\tjobs - print list of jobs - child processes\n");
        printf("\tjobfg <nmb> - resume job nmb in foreground\n");
        printf("\tjobbg <nmb> - resume job nmb in background\n");
        printf("\tjobstop <nmb> - stop job nmb to background\n");
        printf("\tjobsig <nmb> <signal> - send signal to job process\n");
        printf("\tenvvar [name = value] - see envvar list or set name as value\n");
        printf("\te-bash <scriptfile.ebs> - launch e-bash script\n");
        printf("syntax of other calls:\n");
        printf("\t[path/]<programm> [args...] ['|'<samecall>] [&] [> output|< input]\n");
        printf("\tit is execute path/program (or paths from $PATH)\n");
        printf("\twhere samecall:[path/]<programm> [args...] [|<samecall>]\n");
        printf("\tand | it is conveyer chain, & - background launch\n");
        printf("WARNING: developers of e-bash guarantee nothing!\n");
        printf("If you ignore this syntax rules, anything may happen!\n");
        printf("(It is possible to teleport to pole or start nuclear war by wrong syntax)\n");
        printf("You may use help <command> for current information\n");
    }
    else if (strcmp(func, "jobs") == 0 || strcmp(func, "jobfg") == 0 || strcmp(func, "jobbg") == 0 ||
             strcmp(func, "jobstop") == 0 || strcmp(func, "jobsig") == 0 || strcmp(func, "job") == 0)
    {
        printf("family of job... commands:\n");
        printf("\tjobs - print list of jobs - child processes\n");
        printf("\tjobfg <nmb> - resume job nmb in foreground\n");
        printf("\tjobbg <nmb> - resume job nmb in background\n");
        printf("\tjobstop <nmb> - stop job nmb to background\n");
        printf("\tjobsig <nmb> <signal> - send signal to job process\n");
        printf("instructions:\n");
        printf("\tUse jobs for learn number of proccesses.\n");
    }
    else if (strcmp(func, "envvar") == 0)
    {
        printf("\tenvvar - see envvar list or set name as value\n");
        printf("variants:\n");
        printf("\tenvvar - see list of all enviroment variables\n");
        printf("\tenvvar var = value - add or replace variable var with new value\n");
        printf("values of envvars may be:\n");
        printf("\tstring (example: envvar VAR = ABCD)\n");
        printf("\tother var or arithmetic combination of it (envvar VAR = (($TMP + 1)*2)\n");
        printf("You can see \"man environ\"\n");
    }
    else if (strcmp(func, "e-bash") == 0)
    {
        printf("\te-bash - command-line interpretor and utilite for e-bash scripts\n");
        printf("variants:\n");
        printf("\te-bash - launch user-interface application, form shell, s.e\n");
        printf("\te-bash file - launch e-bash script from file (usual *.ebs)\n");
        printf("script rules:\n");
        printf("\tScript it is tuple of usual e-bash commands with some others:\n");
        printf("\tif (rule) {commands...} and while (rule) {commands...} \n");
        printf("\tAll commans, include if (rule) and while (rule), should be\n");
        printf("\tseparated by ENTER (\\n). Variables in script - usual envvars.\n");
        printf("\t//some comment - comments to the end of current string\n");
        printf("script reserved envvars:\n");
        printf("\t $argc, $argv0, $argv1, ... - arg count and args from launch\n");
    }
    else if (strcmp(func, "call") == 0 || strcmp(func, "exec") == 0)
    {
        printf("syntax of other calls:\n");
        printf("\t<conveyer> [&] [> output|< input]\n");
        printf("\tThis call launch a programm\n");
        printf("\twhere conveyer:[path/]<programm> [args...] ['|'<conveyer>]\n");
        printf("describe:\n");
        printf("\tConveyer it is a chain of programms, which will be link by i/o.\n");
        printf("\tAs example, execution of prog1 | prog2 | prog3 it is like this:\n");
        printf("\tstdin -> prog1 -> prog2 -> prog3 -> stdout\n");
        printf("\tIt means, that prog2 will recieve output of prog1 as its input.\n");
        printf("\tIf you put '&' after conveyer, you launch it in a background mode\n");
        printf("\t(you may see help jobs for info about background proccesses);\n");
        printf("\tif you put >/<, conveyer will use your files/envvars instead of stdio.\n");
        printf("and about one part, [path/]<programm> [args...]:\n");
        printf("\tIf you don't fill path, e-bash look it in standart directories.\n");
        printf("WARNING: be careful! Always look man, help, info before launch something!\n");
    }
    else
    {
        printf("I have not any manuals for %s. Try man or info\n", func);
    }
    return 0;
}
