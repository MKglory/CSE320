#ifndef SERVER_HELPER_H
#define SERVER_HELPER_H

// The new custom signal hanlder function
// to handle  disconnected.
// SIGHUP is a signal sent to a process to inform
// it that the terminal or session leader has disconnected.
void sighup_handler(int signo);


// Install a SIGHUP handler so that clean termination of the server can
// be achieved by sending it a SIGHUP.  Note that you need to use
// sigaction() rather than signal(), as the behavior of the latter is
// not well-defined in a multithreaded context.
void setup_SIGHUP();

char buffer[1024];

#endif
