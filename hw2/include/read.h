/*
 * Type definitions for database read functions
 */

Course *readfile(char *root);
Course *readcourse();
Professor *readprofessor();
Assistant *readassistant();
Assignment *readassignments();
Section *readsections(Assignment *a);
Student *readstudents(Assignment *a, Section *sep);
Score *readscores(Assignment *a);
void readgrade(Score *s);
void readweight(Assignment *a);
void readmax(Assignment *a);
void readnorm(Assignment *a);
Surname readsurname();
Name readname();
Atype readatype();
Id readid();
#ifdef MSDOS
int iswhitespace(char c);       /* Correct for buggy ANSI implementation */
#else
int iswhitespace(int c);
#endif
void gobblewhitespace();
void gobbleblanklines();
void advancetoken();
void advanceeol();
void flushtoken();
void expecttoken(char *key);
void expecteof();
void expectnewline();
void pushfile();
void previousfile();
int checktoken(char *key);
int istoken();
int tokensize();
void pushfile();
void fatal(char *fmt, ...);
void error(char* fmt, ...);
void warning(char *fmt, ...);
void debug(char *fmt, ...);

// void fatal(char *fmt, char *a1, char *a2, char *a3, char *a4, char *a5, char *a6);
// void error(char *fmt, char *a1, char *a2, char *a3, char *a4, char *a5, char *a6);
// void warning(char *fmt, char *a1, char *a2, char *a3, char *a4, char *a5, char *a6);
// void debug(char *fmt, char *a1, char *a2, char *a3, char *a4, char *a5, char *a6);