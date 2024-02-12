#include <stdlib.h>

#include "global.h"
#include "debug.h"

static int compare(const char *str1, const char* str2){
    while (*str1 != '\0' && *str2 != '\0'){
        if (*str1 != *str2){
            return 0;
        }
        str1++;
        str2++;
    }
    return (*str1 == '\0' && *str2 == '\0');
}
/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the various options that were specified will be
 * encoded in the global variable 'global_options', where it will be
 * accessible elsewhere in the program.  For details of the required
 * encoding, see the assignment handout.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * @modifies global variable "global_options" to contain an encoded representation
 * of the selected program options.
 */
int validargs(int argc, char **argv)
{   
    global_options = 0x0;
    if (argc < 2){ //by default
        return 0;
    }
    for (int i = 1; i < argc; i++){
        char* arg = *(argv + i);
        // h flag provided
        if (compare(arg, "-h")){
            global_options = HELP_OPTION;
            return 0;
        }
        // m flag provided
        else if (compare(arg, "-m")){
            if (global_options == 0x0 && i == argc - 1)
                global_options = MATRIX_OPTION;
            else
                //if other than h flags is already provided
                return -1;
        }
        //n flag provided
        else if (compare(arg, "-n")){
            if (global_options == 0x0)
                global_options = NEWICK_OPTION;
            else
                //if other than h flags is already provided
                return -1;
        }
        else if (compare(arg, "-o")){
            if (global_options == NEWICK_OPTION || i >= argc-1 || *(*(argv + i + 1)) == '-'){
                //check the outlier name is in correct format
                outlier_name = *(argv + i + 1);
                i++;
            }
            else
                return -1;
        }
    }
    return 0;
}
