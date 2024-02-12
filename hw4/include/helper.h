#ifndef HELPER_H
#define HELPER_H

#include <sys/types.h>
typedef struct process {
    char command[255];
    char args[255];
    char *status;
    pid_t pid;
    PSTATE state;
    PSTATE prev;
    int is_traced;
} PROCESS;

void help_command();
char* extract_command(char* buffer, char** rest);
void sigchld_handler(int signo);
void sigint_handler(int signo);
int add_ID(pid_t IDs[], pid_t PID);
pid_t search_ID(pid_t IDs[], pid_t pid);
void print_process(PROCESS* p);
int debuger(int argc, char *argv[]);
void show_commands();
void show_command(pid_t i);
long str_to_num(char* str);
void error(char* msg);
void state_change(pid_t child_pid, PSTATE new, int status);
int isEmpty(const char* str);
void clear_zombie();
void remove_newline(char* str);
void quit();
void print_backtrace(pid_t child_pid, int limit);
long hex_to_num(char* str);

#endif
