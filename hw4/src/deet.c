#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <ctype.h>
#include "deet.h"
#include "helper.h"

pid_t IDs[255] = {0};
PROCESS processes[255];

void help_command(){
        char* help_list = "Available commands:\nhelp -- Print this help message\nquit (<=0 args) -- Quit the program\nshow (<=1 args) -- Show process info\nrun (>=1 args) -- Start a process\nstop (1 args) -- Stop a running process\ncont (1 args) -- Continue a stopped process\nrelease (1 args) -- Stop tracing a process, allowing it to continue normally\nwait (1-2 args) -- Wait for a process to enter a specified state\nkill (1 args) -- Forcibly terminate a process\npeek (2-3 args) -- Read from the address space of a traced process\npoke (3 args) -- Write to the address space of a traced process\nbt (1 args) -- Show a stack trace for a traced process\n";

    fprintf(stderr, "%s", help_list);
}
// return command
char* extract_command(char* buffer, char** rest){
	char* firstSpace = strchr(buffer, ' ');
    char* command;
    if (firstSpace != NULL){
        //calculate the length of command
        size_t commandL = firstSpace - buffer;
        commandL = commandL > 0 ? commandL : 1;
        command = (char*) malloc((commandL+1) * sizeof(char));
        // copy command from buffer to command
        strncpy(command, buffer, commandL);
        // add NULL terminator
        command[commandL] = '\0';
        *rest = firstSpace + 1;
        return command;
    }else{
        *rest = NULL;
        command = buffer;
        return command;
    }
}

// handler state change
void state_change(pid_t child_pid, PSTATE new, int status){
    int index = search_ID(IDs, child_pid);
    PSTATE old = (processes+index)->state;
    (processes+index)->state = new;
    (processes+index)->prev = old;
    log_state_change(child_pid, old, new, status);
    print_process(processes+index);
}

void error(char* msg){
    if (msg != NULL){
        remove_newline(msg);
        log_error(msg);
    }
    fprintf(stderr, "?\n");
}

int add_ID(pid_t IDs[], pid_t PID){
    for (int i = 0; i < 255; i++){
        if (IDs[i] == 0){
            IDs[i] = PID;
            return i;
        }
    }
    return -1;
}


pid_t search_ID(pid_t IDs[], pid_t pid){
    for (int i = 0; i < 255; i++){
        if (IDs[i] == pid){
            return i;
        }
    }
    return -1;
}

void clear_zombie(){
    for (int i = 0; i < 255; i++){
        if (IDs[i] != 0){
            //check wehther the process is dead
            PROCESS* p = processes + i;
            PSTATE state = p->state;
            if (state == PSTATE_DEAD){
                //processes[i] = NULL;
                IDs[i] = 0;
                // p->command = "";
                // p->args = "";
                memset(p->args, 0, sizeof(p->args));
                memset(p->command, 0, sizeof(p->command));
                p->is_traced = 0;
                p->pid = -1;
                p->status = NULL;
            }
        }
    }
}

void kill_child(pid_t deet_ID){
    pid_t pid = IDs[deet_ID];
    if (pid != 0){
        PROCESS* p = processes+deet_ID;
        if(p->state != PSTATE_NONE && p->state != PSTATE_DEAD && p->state != PSTATE_KILLED){
            // PSTATE new = PSTATE_KILLED; 
            // state_change(pid, new, 0);
            kill(pid, SIGTERM);
            sleep(1);
            PSTATE new = PSTATE_DEAD;
            state_change(pid, new, 0);
        }
    }
    else{
        error(NULL);
    }
}

void quit(){
    clear_zombie();
    int need_printed[255];
    int count = 0;
    // int temp = 1;
    for (int i = 0; i < 255; i++){
        if (IDs[i] != 0){
            //check wehther the process is dead
            pid_t pid = IDs[i];
            PSTATE state = PSTATE_KILLED;
            if (state != PSTATE_DEAD){
                // if (temp == 1){
                //     log_signal(SIGCHLD);
                //     temp--;
                // }
                kill(pid, SIGTERM);
                pid_t child_pid = IDs[i];
                need_printed[count++] = i;
                PSTATE old = (processes+i)->state;
                (processes+i)->state = state;
                log_state_change(child_pid, old, state, 0);          
            }
        }
    }
    for (int i = 0; i < count; i++){
        int index = need_printed[i];
        print_process(processes+index);
    }
    if (count != 0)
        log_signal(SIGCHLD);
    for (int i = 0; i < count; i++){
        pid_t child_pid = IDs[i];
        PSTATE state = PSTATE_DEAD;
        PSTATE old = (processes+i)->state;
        (processes+i)->state = state;
        log_state_change(child_pid, old, state, 0);     
    }
    for (int i = 0; i < count; i++){
        int index = need_printed[i];
        print_process(processes+index);
    }
    log_shutdown();
    exit(EXIT_SUCCESS);
}

int isEmpty(const char* str) {
    if (str == NULL) {
        return 1;  // String is NULL
    }
    // Check if the string contains only whitespace characters
    while (*str != '\0' && *str != '\n' && *str != '\t') {
        if (!isspace((unsigned char)*str)) {
            return 0;  // String has non-whitespace characters
        }
        str++;
    }
    return 1;  // String is either NULL or contains only whitespace characters
}

void remove_newline(char* str){
    if(str != NULL){
        while (*str != '\0'){
            if (*str == '\n'){
                *str = '\0';
                break;
            }
            str++;
        }
    }
}
void print_process(PROCESS* p){
    char* command = p->command;
    char* argument = p->args;
    pid_t pid = p->pid;
    pid_t deetID = search_ID(IDs, pid);
    PSTATE state = p->state;
    char trace;

    // if(ptrace(PTRACE_GETEVENTMSG, pid, 0, 0) == 0){
    if(p->is_traced == 1){
        trace = 'T';
    }
    else{
        trace = 'U';
    }

    printf("%d\t%d\t%c\t", deetID, pid, trace);
    char* current_state;
    if (state == PSTATE_NONE)
        current_state = "none";
    else if (state == PSTATE_RUNNING)
        current_state = "running";
    else if (state == PSTATE_CONTINUING)
        current_state = "continuing";
    else if (state == PSTATE_STOPPED)
        current_state = "stopped";
    else if (state == PSTATE_STOPPING)
        current_state = "stopping";
    else if (state == PSTATE_DEAD)
        current_state = "dead";
    else if(state == PSTATE_KILLED)
        current_state = "killed";

    printf("%s\t%s\t%s ", current_state, p->status, command);
    // printf("running\t%s\t%s ", p->status, command);
    if(!isEmpty(argument))
        printf("%s\n", argument);
    else
        printf("\n");
    fflush(stdout);
    fflush(stderr);

}
void show_commands(){
    int count = 0;
    for (int i = 0; i < 255; i++){
        if (IDs[i] != 0){
            PROCESS* p = processes+i;
            count++;
            print_process(p);
        }
    }
    if (count == 0)
    error(NULL);
}

void show_command(pid_t i){
    if (IDs[i] != 0){
        PROCESS* p = processes+i;
        print_process(p);
    }
    else{
        error(NULL);
    }
}

void print_backtrace(pid_t child_pid, int limit) {
    struct user_regs_struct regs;// register
    unsigned long rbp, rip;

    // // Wait for the child to stop
    // waitpid(child_pid, NULL, 0);

    // Read the value of rbp from the child's register state
    if (ptrace(PTRACE_GETREGS, child_pid, NULL, &regs) == -1) {
        perror("ptrace getregs");
        exit(EXIT_FAILURE);
    }

    rbp = regs.rbp;

    // Follow the chain of stack frames
    // while (rbp != 0x1) {
    int count = limit;
    while (count--){
        // Retrieve the return address
        rip = ptrace(PTRACE_PEEKDATA, child_pid, rbp + sizeof(unsigned long), NULL);
        if (rip == -1) {
            break;
        }
         fprintf(stderr, "%016lx\t%016lx\n", rbp, rip);

        // Move to the next frame
        rbp = ptrace(PTRACE_PEEKDATA, child_pid, rbp, NULL);

        if (rbp == -1) {
            break;
        }

        if (rip == 0x0 || rbp == 0x0) {
            break;
        }
    }
    fflush(stdout);
    fflush(stderr);
}
void sigchld_handler(int signo){
    //int status;
    //pid_t child_pid;
    //log_prompt();
    log_signal(signo);
    pid_t child_pid;
    int status;
    if ((child_pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status)){
            //child process exited normarly
            PSTATE new = PSTATE_DEAD;
            state_change(child_pid, new, status);
        }
        else if(WIFSIGNALED(status)){
            //the child process was terminated by a signal,
            PSTATE new = PSTATE_KILLED;
            state_change(child_pid, new, status);
        }
        else if (WIFSTOPPED(status)){
            //the child process was stopped by a signal,
            PSTATE new = PSTATE_STOPPED;
            state_change(child_pid, new, status);
        }
        else if (WIFCONTINUED(status)) {
            PSTATE new = PSTATE_CONTINUING;
            state_change(child_pid, new, status);
        }
        // else{
        //     printf("something\n");
        // }
    }
    fflush(stdout);
    fflush(stderr);
}

void sigint_handler(int signo){
    //log_prompt();
    //log_signal(signo);
    //printf("hanlder: received SIGINT\n");
    log_signal(signo);
    quit();
    //sleep(0.5);
    exit(EXIT_SUCCESS);
}

long str_to_num(char* str){
    char *endptr;
    long result = strtol(str, &endptr, 10);
    return result;
}
long hex_to_num(char* str){
    char *endptr;
    long result = strtol(str, &endptr, 16);
    return result;  
}

void wait_child_signal(){
    int status;
    pid_t child_pid;
    if ((child_pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
         if (WIFEXITED(status)){
            //child process exited normarly
            PSTATE new = PSTATE_DEAD;
            state_change(child_pid, new, status);
        }
        else if(WIFSIGNALED(status)){
            //the child process was terminated by a signal,
            PSTATE new = PSTATE_KILLED;
            state_change(child_pid, new, status);
        }
        else if (WIFSTOPPED(status)){
            //the child process was stopped by a signal,
            PSTATE new = PSTATE_STOPPED;
            state_change(child_pid, new, status);
        }
        else if (WIFCONTINUED(status)) {
            PSTATE new = PSTATE_CONTINUING;
            state_change(child_pid, new, status);
        }
        // else{
        //     printf("something\n");
        // }
    }
}
void wait_a_child(pid_t child_pid){
    int status;
    if ((waitpid(child_pid, &status, 0)) > 0) {
         if (WIFEXITED(status)){
            //child process exited normarly
            PSTATE new = PSTATE_DEAD;
            state_change(child_pid, new, status);
        }
        else if(WIFSIGNALED(status)){
            //the child process was terminated by a signal,
            PSTATE new = PSTATE_KILLED;
            state_change(child_pid, new, status);
        }
        else if (WIFSTOPPED(status)){
            //the child process was stopped by a signal,
            PSTATE new = PSTATE_STOPPED;
            state_change(child_pid, new, status);
        }
        else if (WIFCONTINUED(status)) {
            PSTATE new = PSTATE_CONTINUING;
            state_change(child_pid, new, status);
        }
        // else{
        //     printf("something\n");
        // }
    }
}
int debuger(int argc, char *argv[]){
    //check -p
    int suppresed = 0;
    if (argc > 1){
        if (strcmp(argv[1], "-p") == 0)
            suppresed = 1;
    }
    //handler for SIGCHLD
    struct sigaction sigchld_action;
    sigchld_action.sa_handler = sigchld_handler;
    sigemptyset(&sigchld_action.sa_mask);
    sigchld_action.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sigchld_action, NULL);

    //handler for SIGINT
    struct sigaction sigint_action;
    sigint_action.sa_handler = sigint_handler;
    sigemptyset(&sigint_action.sa_mask);
    sigint_action.sa_flags = 0;
    sigaction(SIGINT, &sigint_action, NULL);


    //intialize program, startup
    log_startup();
    size_t len = 0;
    char* buffer = NULL;
    while (1){
        // display prompt
        fflush(stdout);
        fflush(stderr);
        log_prompt();
        if (!suppresed)  fprintf(stderr, "deet> ");
        wait_child_signal();
        //check the available commands
        errno = 0;
        //sleep(1);
        if (getline(&buffer, &len, stdin) == -1){
            //if read input error
            if(feof(stdin)){ // CTRL-D
                quit();
            }
            //Clear any error and continue to the next iteration
            errno = 0;
            continue;
        }
        else{
            log_input(buffer);
            // take command
            char* args;
            char* command = extract_command(buffer, &args);

            if (command != NULL){
                if (strcmp(command, "help\n") == 0 || strcmp(command, "help") == 0){
                    help_command();
                }
                else if (strcmp(command, "quit\n") == 0 || strcmp(command, "quit") == 0){
                    quit();
                }
                else if (strcmp(command, "show\n") == 0 || strcmp(command, "show") == 0){   
                    //show all exited process
                    if (args != NULL){
                        pid_t chosen_deetID = (pid_t)str_to_num(args);
                        show_command(chosen_deetID);
                    }
                    else{
                        show_commands();
                    }
                }
                else if (strcmp(command, "cont") == 0 || strcmp(command, "cont\n") == 0 ){
                    free(command);
                    if (!isEmpty(args)){
                        pid_t chosen_deetID = (pid_t)str_to_num(args);
                        pid_t pid = IDs[chosen_deetID];
                        // check is this process current stopped
                        PROCESS* p = processes+chosen_deetID;
                        if (p->state == PSTATE_STOPPED){
                            if(ptrace(PTRACE_CONT, pid,0,0) != -1){;
                                PSTATE new = PSTATE_RUNNING;
                                state_change(pid, new, 0);
                                p->status = "0x0"; //contiue and then dead
                                wait_child_signal();
                            }
                        }
                        else{
                            error(NULL);
                        }
                    }
                    else{
                        error(NULL);
                    }
                }
                else if (strcmp(command, "stop") == 0){
                    free(command);
                    if (!isEmpty(args)){
                        pid_t chosen_deetID = (pid_t)str_to_num(args);
                        if (chosen_deetID == -1)
                            chosen_deetID = 0;
                        pid_t pid = IDs[chosen_deetID];
                        // check is this process current stopped
                        PROCESS* p = processes+chosen_deetID;
                        if(p->state != PSTATE_NONE && p->state != PSTATE_DEAD && p->state != PSTATE_STOPPED && p->state != PSTATE_KILLED){
                            PSTATE new = PSTATE_STOPPING;
                            state_change(pid, new, 0);
                            kill(pid, SIGSTOP);
                            // if (ptrace(PTRACE_INTERRUPT, pid, NULL, NULL) == -1) {
                            //     perror("ptrace");
                            //     exit(EXIT_FAILURE);
                            // }
                            // state_change(pid, new, 0);
                        }
                        else{
                            error(NULL);
                        }
                    }
                }
                else if (strcmp(command, "release") == 0){
                    //to do
                    free(command);
                    if (!isEmpty(args)){
                        char* token = strtok(args, " ");
                        //first argument is the Deet ID
                        pid_t chosen_deetID = (pid_t)str_to_num(token);
                        pid_t pid = IDs[chosen_deetID];
                        if(ptrace(PTRACE_DETACH, pid, NULL, NULL) != -1){
                            (processes+chosen_deetID)->is_traced = 0;
                            PSTATE new = PSTATE_RUNNING;
                            state_change(pid, new, 0);
                            //wait_child_signal();
                        }
                    }
                    else{
                        error(NULL);
                    }
                }
                else if (strcmp(command, "wait") == 0){
                    free(command);
                    if (!isEmpty(args)){
                        char* token = strtok(args, " ");
                        //first argument is the Deet ID
                        pid_t chosen_deetID = (pid_t)str_to_num(token);
                        pid_t pid = IDs[chosen_deetID];
                        token = strtok(args, " ");
                        //check second argument
                        int status;
                        if(strcmp(token, "0")){ //only one argument
                            while (1){
                                if(waitpid(pid, &status, 0) == -1){
                                    perror("waitpid");
                                    break;
                                    exit(EXIT_FAILURE);
                                }
                                if (WIFEXITED(status)) {
                                    //child process exited normarly
                                    PSTATE new = PSTATE_DEAD;
                                    state_change(pid, new, status);
                                    break;
                                } 
                                else if (WIFSIGNALED(status)) {
                                    //the child process was terminated by a signal,
                                    PSTATE new = PSTATE_KILLED;
                                    state_change(pid, new, status);
                                    break;
                                }
                            }
                        }
                        else{
                            // PSTATE desired_state;
                            // if (strcmp(token, "running")){
                            //     desired_state = PSTATE_RUNNING;
                            // }
                            // else if (strcmp(token, "stopping")){
                            //     desired_state = PSTATE_STOPPING;
                            // }
                            // else if (strcmp(token, "stopped")){
                            //     desired_state = PSTATE_STOPPED;
                            // }
                            // else if (strcmp(token, "continuing")){
                            //     desired_state = PSTATE_CONTINUING;
                            // }
                            // else if (strcmp(token, "killed")){
                            //     desired_state = PSTATE_KILLED;
                            // }
                            // else if (strcmp(token, "dead")){
                            //     desired_state = PSTATE_DEAD;
                            // }
                            while (1){
                                if(waitpid(pid, &status, 0) == -1){
                                    perror("waitpid");
                                    break;
                                    exit(EXIT_FAILURE);
                                }
                                if (WIFEXITED(status)) {
                                    //child process exited normarly
                                    PSTATE new = PSTATE_DEAD;
                                    state_change(pid, new, status);
                                    break;
                                } 
                                else if (WIFSIGNALED(status)) {
                                    //the child process was terminated by a signal,
                                    PSTATE new = PSTATE_KILLED;
                                    state_change(pid, new, status);
                                    break;
                                }
                            }
                        }
                    }
                }
                else if (strcmp(command, "kill") == 0){
                    free(command);
                    if (!isEmpty(args)){
                        pid_t chosen_deetID = (pid_t)str_to_num(args);
                        // check is this process current stopped
                        PROCESS* p = processes+chosen_deetID;
                        p->status = "0x9";
                        if(p->state != PSTATE_NONE && p->state != PSTATE_DEAD && p->state != PSTATE_KILLED){
                            kill_child(chosen_deetID);

                        }
                    }
                }
                else if (strcmp(command, "peek") == 0){
                    free(command);
                    if (!isEmpty(args)){
                        char* token = strtok(args, " ");
                        pid_t deetID = (pid_t)str_to_num(token);
                        pid_t pid = IDs[deetID];

                        token = strtok(NULL, " ");
                        char* hex;
                        size_t addr;
                        int num_words = -1;
                        if (!isEmpty(token)){
                            hex = token;
                            addr = hex_to_num(hex);
                            token = strtok(NULL, " ");
                                if (!isEmpty(token)){
                                    num_words = (int)str_to_num(token);
                                }
                        }
                        unsigned long data;
                        data = ptrace(PTRACE_PEEKDATA, pid, (void*)addr, NULL);

                        if (num_words > 0){
                            for (int i = 0; i < num_words; ++i) {
                                if (data == -1) {
                                    error(NULL);
                                }
                                else{
                                    fprintf(stderr, "%016lx\t%016lx\n", addr, data);        
                                    data = ptrace(PTRACE_PEEKDATA, pid, (void*)addr + sizeof(unsigned long), NULL);
                                    addr += sizeof(unsigned long);
                                    fflush(stdout);
                                    fflush(stderr);
                                }
                            }
                        }
                        else{
                            if (data == -1) {
                                error(NULL);
                            }
                            else{
                                fprintf(stderr, "%016lx\t%016lx\n", addr, data);
                                fflush(stdout);
                                fflush(stderr);
                            }
                            
                        }
                        //
                    }else{
                        error(NULL);
                    }
                }
                else if (strcmp(command, "poke") == 0){
                    free(command);
                    if (!isEmpty(args)){
                        char* token = strtok(args, " ");
                        pid_t deetID = (pid_t)str_to_num(token);
                        pid_t pid = IDs[deetID];

                        token = strtok(NULL, " ");
                        char* hex;
                        size_t addr;
                        long value = -1;
                        if (!isEmpty(token)){
                            hex = token;
                            addr = hex_to_num(hex);
                            token = strtok(NULL, " ");
                                if (!isEmpty(token)){
                                    value = (long)str_to_num(token);
                                }else{
                                    error(NULL);
                                }
                        }

                        else{
                            error(NULL);
                        }
                        ptrace(PTRACE_POKEDATA, pid, (void *)addr, (void *)value);
                    }else{
                        error(NULL);
                    }
                }
                else if (strcmp(command, "bt") == 0){
                    int limit = 10;
                    free(command);
                    if (!isEmpty(args)){
                        char* token = strtok(args, " ");
                        pid_t chosen_deetID = (pid_t)str_to_num(token); 
                        token = strtok(NULL, " ");
                        if (token != NULL){
                            int size = (int)str_to_num(token);
                            if (size == 0){
                                size = limit;
                            }
                            limit = size;
                        }
                        pid_t pid = IDs[chosen_deetID];
                        print_backtrace(pid, limit);
                    }
                    else{
                        error(NULL);
                    }
                }
                else if (strcmp(command, "run") == 0){
                    if(!isEmpty(args)){
                        free(command);
                        pid_t child_pid = fork();

                        char* arguments; //argument after program 
                        char* unix_command = extract_command(args, &arguments); //program
                        //check the argument number

                        clear_zombie();
                        //initialize a new process
                        int index;
                        pid_t add_id = child_pid;
                        if (child_pid == 0){// child process
                            add_id = getpid();
                        }
                        index = add_ID(IDs, add_id);
                        processes[index].pid = add_id;
                        (processes+index)->is_traced = 1;
                        (processes+index)->status = "";

                        size_t command_length = strlen(unix_command);
                        size_t args_length;
                        if (isEmpty(arguments)){
                            args_length = 0;
                            remove_newline(unix_command);
                        }
                        else{
                            args_length = strlen(arguments);
                            //remove the newline symbol
                            arguments[args_length-1] = '\0';

                            strncpy((processes+index)->args, arguments, args_length);
                            (processes + index)->args[args_length] = '\0';  // Null-terminate the string
                        }

                        strncpy((processes+index)->command, unix_command, command_length);
                        (processes + index)->command[command_length] = '\0';  // Null-terminate the string
                        if ((processes+index)->state)
                            //initliaze is always none
                            (processes+index)->state = PSTATE_NONE; 
                        if (child_pid == -1){
                            exit(EXIT_FAILURE);
                        }
                        if (child_pid == 0){
                            //child process
                            // Redirect stdout to stderr
                            if(dup2(STDERR_FILENO, STDOUT_FILENO) == -1){
                                exit(EXIT_FAILURE);
                            }
                            pid_t current_pid = getpid();
                            pid_t deet_ID = search_ID(IDs, current_pid);
                            // Request tracing by parent
                            if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1){
                                exit(EXIT_FAILURE);
                            }

                            if (deet_ID == -1){
                                exit(EXIT_FAILURE);
                            }
                            PROCESS* current = processes + deet_ID;
                            char* unix_command = current->command;
                            char* arguments = current->args;
                            //Execute the command
                            char* unix_args[] = {unix_command, arguments, NULL};
                            current->status = "0x0";
                            sleep(1);
                            execvp(unix_command, unix_args);
                            exit(EXIT_FAILURE);
                        }

                        else{
                            //parent process
                            //in run command, it always create a new process
                            PSTATE new = PSTATE_RUNNING;
                            state_change(child_pid, new, 0);

                            //sleep(1);
                            wait_a_child(child_pid);
                        }
                    }
                    else{ // agurment is null
                        error(NULL);
                    }
                }
                else{
                    error(command);
                    //free(command);
                }
            }
        }
    }
    free(buffer);
    return 0;
}



