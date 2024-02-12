#include <stdio.h>
#include <criterion/criterion.h>
#include "gradedb.h"
#include "read.h"
#include "write.h"
#include "sort.h"
#include "stats.h"
#include "test_common.h"

#define TEST_INPUT "cse307.dat"

extern int errors, warnings;

Test(basecode_suite, read_file) {
    char *name = "read_file";
    setup_test(name);

    Course *c;
    c = readfile(test_infile);
    cr_assert_eq(errors, 0, "There were errors reported when reading test data.\n");
    cr_assert_neq(c, NULL, "NULL pointer returned from readfile().\n");
}

Test(basecode_suite, stats) {
    char *name = "stats";
    setup_test(name);

    Course *c;
    Stats *s;
    c = readfile(test_infile);
    cr_assert_eq(errors, 0, "There were errors reported when reading test data.\n");
    cr_assert_neq(c, NULL, "NULL pointer returned from readfile().\n");
    s = statistics(c);
    cr_assert_neq(s, NULL, "NULL pointer returned from statistics().\n");
}

Test(basecode_suite, collate) {
    char *name = "collate";
    setup_test(name);

    Course *c;
    c = readfile(test_infile);
    cr_assert_eq(errors, 0, "There were errors reported when reading test data.\n");
    cr_assert_neq(c, NULL, "NULL pointer returned from readfile().\n");
    FILE *f = fopen(test_outfile, "w");
    cr_assert_neq(f, NULL, "Error opening test output file (%s).\n", test_outfile);
    statistics(c);
    sortrosters(c, comparename);
    writecourse(f, c);
    fclose(f);
    assert_outfile_matches(name, NULL);
}

Test(basecode_suite, tabsep) {
    char *name = "tabsep";
    setup_test(name);
    extern void reporttabs(FILE *fd, Course *c);
    extern void normalize(Course *c, Stats *s);
    extern void composites(Course *c);

    Course *c;
    Stats *s;
    c = readfile(test_infile);
    cr_assert_eq(errors, 0, "There were errors reported when reading test data.\n");
    cr_assert_neq(c, NULL, "NULL pointer returned from readfile().\n");
    s = statistics(c);
    cr_assert_neq(s, NULL, "NULL pointer returned from statistics().\n");
    normalize(c, s);
    composites(c);
    sortrosters(c, comparename);
    FILE *f = fopen(test_outfile, "w");
    cr_assert_neq(f, NULL, "Error opening test output file.\n");
    reporttabs(f, c);
    fclose(f);
    assert_outfile_matches(name, NULL);
}

Test(basecode_suite, blackbox_help) {
    char *name = "blackbox_help";
    setup_test(name);
    int err = run_using_system(name, "", "", "-h", STANDARD_LIMITS);
    assert_expected_status(EXIT_FAILURE, err);  // -h is not a valid option
    assert_errfile_matches(name, NULL);
}

Test(basecode_suite, blackbox_report) {
    char *name = "blackbox_report";
    setup_test(name);
    int err = run_using_system(name, "", "", "-r tests/rsrc/cse307.dat", STANDARD_LIMITS);
    assert_expected_status(EXIT_SUCCESS, err);
}
