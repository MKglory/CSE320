/*
 * DO NOT MODIFY THE CONTENTS OF THIS FILE.
 * IT WILL BE REPLACED DURING GRADING
 */
#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdio.h>

/*
 * USAGE macro to be called from main() to print a help message and exit
 * with a specified exit status.
 */
#define USAGE(program_name, retcode) do { \
fprintf(stderr, "USAGE: %s %s\n", program_name, \
"[-h] [-m|-n] [-o <name>]\n" \
"   -h         Help: displays this help menu.\n" \
"   -m         Output matrix of estimated distances, instead of edge data.\n" \
"   -n         Output tree in Newick format, instead of edge data.\n" \
"   -o <name>  Use <name> as the name of the outlier node to use for Newick output\n" \
"              (only permitted if -n has already appeared).\n" \
"\n" \
"If -h is specified, then it must be the first option on the command line, and any\n"\
"other options are ignored.\n" \
"\n" \
"If -h is not specified, then the program reads distance data from the standard input,\n" \
"and synthesizes an unrooted tree using the neighbor joining method.  The default\n" \
"behavior of the program is to output the edges in the synthesized tree to the standard output.\n" \
"\n" \
"If -m is specified, then the final matrix of estimated node distances is output to\n" \
"to the standard output, instead of the edge data.\n" \
"\n" \
"If -n is specified, then a representation of the synthesized tree in Newick format is output\n" \
"to the standard output, instead of the edge data.  The -o option, which is only permitted\n" \
"after -n, is used to specify the name of an 'outlier' node to be used in constructing a rooted\n" \
"tree for Newick output.\n" \
"\n" \
); \
exit(retcode); \
} while(0)

/*
 * Options info, set by validargs.
 *   If -h is specified, then the HELP_OPTION bit is set.
 */
long global_options;

/*
 * Bits that are OR-ed in to global_options to specify various modes of
 * operation.
 */
#define HELP_OPTION      (0x00000001)
#define NEWICK_OPTION    (0x00000002)
#define MATRIX_OPTION    (0x00000004)

/* Name of a leaf node to be used as an "outlier", otherwise NULL. */
char *outlier_name;

/* Maximum size of an input field (taxon name or distance). */
#define INPUT_MAX 100

/*
 * Buffer to use while reading an input field.  There is one entry
 * for each character of input, plus one additional entry to hold
 * a null character ('\0') as required to turn the contents into a C string.
 */
char input_buffer[INPUT_MAX+1];

/* Maximum number of taxa (leaf nodes) that can be handled. */
#define MAX_TAXA 100

/* Number of taxa in input file. */
int num_taxa;

/*
 * Maximum number of nodes that can be created.
 * The algorithm runs for num_taxa - 2 iterations.
 * At each iteration a node is created.
 * So there can be at most MAX_TAXA - 2 nodes created,
 * plus at most MAX_TAXA leaf nodes.
 */
#define MAX_NODES (2 * MAX_TAXA - 2)

/* Current number of nodes (leaf + internal). */
int num_all_nodes;

/* Names associated with nodes. */
char node_names[MAX_NODES][INPUT_MAX+1];

/* Inter-node distances. */
double distances[MAX_NODES][MAX_NODES];

/* Row sums of distances matrix. */
double row_sums[MAX_NODES];

/* Current number of nodes that have not yet been joined. */
int num_active_nodes;

/*
 * Table mapping indices of active nodes (in [0, num_active_nodes))
 * to indices of all nodes (in [0, num_all_nodes)).
 * This is used to make it possible to remove the nodes joined at
 * each iteration without lots of recopying.
 */
int active_node_map[MAX_NODES];

/*
 * Nodes for a data structure to represent an unrooted tree.
 * Each node (whether leaf or internal) is represented by a NODE
 * structure.  The "name" field is set to point to the name of
 * the node, which is stored in a row of the "node_names" array.
 * The "neighbors" field is a three-element array whose elements
 * point to adjacent nodes in the tree.
 * For a leaf node, there is just one adjacent node, which is
 * pointed at by neighbors[0], and other two entries are NULL.
 * For an internal node, there are exactly three adjacent nodes,
 * so all three entries contain a valid pointer.  
 * Since the final tree is unrooted, there is ultimately no particular
 * distinction ("parent" or "child") among the three neighbors of
 * an internal node.  However, as you are creating the tree, you
 * should store the "parent pointer" of a node in neighbors[0]
 * and the "child pointers" in neighbors[1] and neighbors[2].
 * The single edge added as the last step in the algorithm should
 * be stored in the neighbors[0] field of both of the nodes that
 * are being connected.
 */

typedef struct node {
    char *name;
    struct node *neighbors[3];
} NODE;

/* Array containing storage for NODE structures. */
NODE nodes[MAX_NODES];

/*
 * Function you are to implement that validates and interprets command-line arguments
 * to the program.  See the stub in validargs.c for specifications.
 */
extern int validargs(int argc, char **argv);

/*
 * Functions you are to implement that perform the main functions of the program.
 * See the assignment handout and the comments in front of the stub for each function
 * in philo.c for full specifications.
 */

extern int read_distance_data(FILE *in);
extern int build_taxonomy(FILE *out);
extern int emit_newick_format(FILE *out);
extern int emit_distance_matrix(FILE *out);

#endif
