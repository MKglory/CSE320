#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>


#include "deet.h"
#include "helper.h"

int main(int argc, char *argv[]) {
    // TO BE IMPLEMENTED
    // Remember: Do not put any functions other than main() in this file.
    debuger(argc, argv);
    return 0;
}
