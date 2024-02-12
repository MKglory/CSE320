/*
 * Error handling routines
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

int errors;
int warnings;
int dbflag = 1;

void fatal(char *fmt, ...)
{
        va_list args;
        va_start(args, fmt);
        fprintf(stderr, "\nFatal error: ");
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
        exit(1);
}

void error(char* fmt, ...)
{
        va_list args;
        va_start(args, fmt);
        fprintf(stderr, "\nError: ");
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
        errors++;
}

void warning(char *fmt, ...)
{
        va_list args;
        va_start(args, fmt);
        fprintf(stderr, "\nWarning: ");
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
        warnings++;
}

void debug(char *fmt, ...)
{
        if(!dbflag) return;
        va_list args;
        va_start(args, fmt);
        fprintf(stderr, "\nDebug: ");
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
}
