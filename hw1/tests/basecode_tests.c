#include <sys/wait.h>
#include <criterion/criterion.h>
#include <criterion/logging.h>

#include "global.h"

#define progname "bin/philo"

Test(basecode_suite, validargs_help_test, .timeout = 5) {
    char *argv[] = {progname, "-h", NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int opt = global_options;
    int flag = HELP_OPTION;
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
    cr_assert_eq(opt & flag, flag, "Correct bit (0x%x) not set for -h. Got: %x",
		 flag, opt);
}

Test(basecode_suite, validargs_no_flags_test, .timeout = 5) {
    char *argv[] = {progname, NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int opt = global_options;
    int exp_opt = 0;
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
    cr_assert_eq(opt, exp_opt, "Invalid options settings.  Got: 0x%x | Expected: 0x%x",
		 opt, exp_opt);
}

Test(basecode_suite, validargs_matrix_test, .timeout = 5) {
    char *argv[] = {progname, "-m", NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int opt = global_options;
    int exp_opt = MATRIX_OPTION;
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
    cr_assert_eq(opt, exp_opt, "Invalid options settings.  Got: 0x%x | Expected: 0x%x",
		 opt, exp_opt);
}

Test(basecode_suite, validargs_newick_test, .timeout = 5) {
    char *outlier = "xyz";
    char *argv[] = {progname, "-n", "-o", outlier, NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int opt = global_options;
    int exp_opt = NEWICK_OPTION;
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
    cr_assert_eq(opt, exp_opt, "Invalid options settings.  Got: 0x%x | Expected: 0x%x",
		 opt, exp_opt);
    int cmp = strcmp(outlier_name, outlier);
    cr_assert_eq(cmp, 0, "Outlier name not properly set.  Got: '%s' | Expected: '%s'",
		 outlier_name, outlier);
}

Test(basecode_suite, validargs_error_test, .timeout = 5) {
    char *argv[] = {progname, "-m", "-o", "xyz", NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int exp_ret = -1;
    int ret = validargs(argc, argv);
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
}

Test(basecode_suite, help_system_test, .timeout = 5) {
    char *cmd = "bin/philo -h > /dev/null 2>&1";

    // system is a syscall defined in stdlib.h
    // it takes a shell command as a string and runs it
    // we use WEXITSTATUS to get the return code from the run
    // use 'man 3 system' to find out more
    int return_code = WEXITSTATUS(system(cmd));

    cr_assert_eq(return_code, EXIT_SUCCESS,
                 "Program exited with 0x%x instead of EXIT_SUCCESS",
		 return_code);
}

Test(basecode_suite, philo_basic_test, .timeout = 5) {
    char *cmd = "bin/philo < rsrc/wikipedia.csv > test_output/philo_basic_test.out";
    char *cmp = "cmp test_output/philo_basic_test.out rsrc/wikipedia_edges.out";

    int return_code = WEXITSTATUS(system(cmd));
    cr_assert_eq(return_code, EXIT_SUCCESS,
                 "Program exited with 0x%x instead of EXIT_SUCCESS",
		 return_code);
    return_code = WEXITSTATUS(system(cmp));
    cr_assert_eq(return_code, EXIT_SUCCESS,
                 "Program output did not match reference output.");
}
