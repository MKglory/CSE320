#include <stdlib.h>

#include "global.h"
#include "debug.h"

//function prototype declaration
int computeQ(int i, int j);
int update_matrix_with_new_node(int i, int j);
int minimum_Q_Value(int* i, int* j);
int compute_edge_data(int i, int j, int indice_new_node, FILE* out, int printable);
void update_sum_rows();
void initialize_row_sums();
int parent_index(char** parent_name, int current_node_index);
int is_leaf(int node_index);
static int compare(char *str1, char* str2);
static int compare(char *str1, char* str2){
    if(str1 == NULL || str2 == NULL){
        return 0;
    }
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
 * @brief  Read genetic distance data and initialize data structures.
 * @details  This function reads genetic distance data from a specified
 * input stream, parses and validates it, and initializes internal data
 * structures.
 *
 * The input format is a simplified version of Comma Separated Values
 * (CSV).  Each line consists of text characters, terminated by a newline.
 * Lines that start with '#' are considered comments and are ignored.
 * Each non-comment line consists of a nonempty sequence of data fields;
 * each field iis terminated either by ',' or else newline for the last
 * field on a line.  The constant INPUT_MAX specifies the maximum number
 * of data characters that may be in an input field; fields with more than
 * that many characters are regarded as invalid input and cause an error
 * return.  The first field of the first data line is empty;
 * the subsequent fields on that line specify names of "taxa", which comprise
 * the leaf nodes of a phylogenetic tree.  The total number N of taxa is
 * equal to the number of fields on the first data line, minus one (for the
 * blank first field).  Following the first data line are N additional lines.
 * Each of these lines has N+1 fields.  The first field is a taxon name,
 * which must match the name in the corresponding column of the first line.
 * The subsequent fields are numeric fields that specify N "distances"
 * between this taxon and the others.  Any additional lines of input following
 * the last data line are ignored.  The distance data must form a symmetric
 * matrix (i.e. D[i][j] == D[j][i]) with zeroes on the main diagonal
 * (i.e. D[i][i] == 0).
 *
 * If 0 is returned, indicating data successfully read, then upon return
 * the following global variables and data structures have been set:
 *   num_taxa - set to the number N of taxa, determined from the first data line
 *   num_all_nodes - initialized to be equal to num_taxa
 *   num_active_nodes - initialized to be equal to num_taxa
 *   node_names - the first N entries contain the N taxa names, as C strings
 *   distances - initialized to an NxN matrix of distance values, where each
 *     row of the matrix contains the distance data from one of the data lines
 *   nodes - the "name" fields of the first N entries have been initialized
 *     with pointers to the corresponding taxa names stored in the node_names
 *     array.
 *   active_node_map - initialized to the identity mapping on [0..N);
 *     that is, active_node_map[i] == i for 0 <= i < N.
 *
 * @param in  The input stream from which to read the data.
 * @return 0 in case the data was successfully read, otherwise -1
 * if there was any error.  Premature termination of the input data,
 * failure of each line to have the same number of fields, and distance
 * fields that are not in numeric format should cause a one-line error
 * message to be printed to stderr and -1 to be returned.
 */
int read_distance_data(FILE *in) {
    // TO BE IMPLEMENTED
    int buffer_index = 0;
    int ch;
    int line = 0; // the vertical index
    num_taxa = 0;
    int i = 0; //Counter for node_names array
    int j = 0; //Counter for distance 2D array
    while ((ch = fgetc(in)) != EOF){
        //check for a line starting with '#'
        if (ch == '#'){
            while ((ch = fgetc(in)) != EOF && ch != '\n'){
                //skip characters until end of the line
            }
            continue;
        }
        line++; //line started with # wouldn't counted
        ungetc(ch, in);
        //this line specify names of "taxa" which comprise the leaf nodes of a phylogenetuc tree
        if (line == 1){
            i = 0; //countor for node_names array set to 0
            while ((ch = fgetc(in)) != EOF && ch != '\n'){
                if (ch == ',') {
                    //each filed is terminated by ',' and new line
                    buffer_index = 0; //ready to read another field
                    num_taxa++;
                    if (num_taxa > MAX_TAXA){
                        return EXIT_FAILURE;
                    }
                    i = 0;
                }
                else{
                    if (buffer_index >= INPUT_MAX){
                        return EXIT_FAILURE;
                    }
                    *(input_buffer + buffer_index++) = ch;
                    // node_names[num_taxa-1][i++] = ch;
                    *(*(node_names + num_taxa - 1) + i++) = ch;
                    // node_names[num_taxa-1][i] = '\0';
                    *(*(node_names + num_taxa - 1) + i) = '\0';
                    if (buffer_index >= INPUT_MAX){
                        return EXIT_FAILURE;
                    }
                    *(input_buffer + buffer_index++) = '\0';
                }
            }
            i = 0;
        }
        else {
            j = -1; //counter for distance 2d array
            ch = fgetc(in);
            int boolean_decimal = 0;
            int decimal_index = 0;
            while ((ch = fgetc(in)) != EOF && ch != '\n'){
                if (ch == ','){
                    buffer_index = 0;
                    boolean_decimal = 0; //reset mode
                    decimal_index = 0; //reset
                    //skip ','
                    j++;
                }
                else if (ch == '.'){
                    boolean_decimal = 1;
                }
                else {
                    //distances[i][j++] = ch;
                    if (*(*(distances + i) + j) == 0){
                        if (buffer_index >= INPUT_MAX){
                            return EXIT_FAILURE;
                        }
                        *(input_buffer + buffer_index) = ch - 48;
                        *(*(distances + i) + j) = ch - 48;
                    }
                    else{
                        if (buffer_index >= INPUT_MAX){
                            return EXIT_FAILURE;
                        }

                        if (boolean_decimal == 1){
                            decimal_index++;
                            double decimal = ch - 48;
                            if (decimal_index < 4){
                                for (int i = 0; i < decimal_index; i++){
                                    decimal = decimal / 10.0;
                                }
                                *(input_buffer + buffer_index++) += decimal;
                                *(*(distances + i) + j) += decimal;
                            }
                        }   
                        else{
                            *(input_buffer + buffer_index) *= 10;
                            *(input_buffer + buffer_index++) += ch - 48;
                            *(*(distances + i) + j) *= 10; //minus the 0 value in ASCII
                            *(*(distances + i) + j) += ch - 48;
                        }
                    }
                }
            }
            buffer_index++;
            i++;
            //j = 0;
        }
        // Any additional lines input that more than num_taxa is ignored
        if (line > num_taxa){
            break;
        }
    }
    num_all_nodes = num_taxa;
    num_active_nodes = num_taxa;
    for (int i = 0; i < num_taxa; i++){
        //fill up nodes
        (nodes + i)->name = *(node_names + i);
        //active_node_map[i] = i;
        *(active_node_map + i) = i;
        //set diagonal to 0
         *(*(distances + i) + i) = 0;
    }
    return 0;
}

/**
 * @brief  Emit a representation of the phylogenetic tree in Newick
 * format to a specified output stream.
 * @details  This function emits a representation in Newick format
 * of a synthesized phylogenetic tree to a specified output stream.
 * See (https://en.wikipedia.org/wiki/Newick_format) for a description
 * of Newick format.  The tree that is output will include for each
 * node the name of that node and the edge distance from that node
 * its parent.  Note that Newick format basically is only applicable
 * to rooted trees, whereas the trees constructed by the neighbor
 * joining method are unrooted.  In order to turn an unrooted tree
 * into a rooted one, a root will be identified according by the
 * following method: one of the original leaf nodes will be designated
 * as the "outlier" and the unique node adjacent to the outlier
 * will serve as the root of the tree.  Then for any other two nodes
 * adjacent in the tree, the node closer to the root will be regarded
 * as the "parent" and the node farther from the root as a "child".
 * The outlier node itself will not be included as part of the rooted
 * tree that is output.  The node to be used as the outlier will be
 * determined as follows:  If the global variable "outlier_name" is
 * non-NULL, then the leaf node having that name will be used as
 * the outlier.  If the value of "outlier_name" is NULL, then the
 * leaf node having the greatest total distance to the other leaves
 * will be used as the outlier.
 *
 * @param out  Stream to which to output a rooted tree represented in
 * Newick format.
 * @return 0 in case the output is successfully emitted, otherwise -1
 * if any error occurred.  If the global variable "outlier_name" is
 * non-NULL, then it is an error if no leaf node with that name exists
 * in the tree.
 */

int parent_index(char** parent_name, int current_node_index){
    //find parent of outlier real name and index
    int parent_index;
    *parent_name = (*((nodes+current_node_index)->neighbors))->name; 
    for (int j = 0; j < num_all_nodes; j++){
        if (compare(*parent_name, *(node_names+j)))
            parent_index = j;
    }
    return parent_index;
}

int is_leaf(int node_index){
    for (int i = 1; i < 3; i++){
        if ((*((nodes+node_index)->neighbors + i)) != NULL){
            return 0;
        }
    }
    return 1;
}

void newick_process(int node_indice, int known_node_indice){
    char* node_name = *(node_names + node_indice);
    char* known_node_name = *(node_names + known_node_indice);
    double edge = *(*(distances+node_indice) + known_node_indice);
    //check the node_indice is leaf or not
    if (!is_leaf(node_indice)){
        fprintf(stdout, "(");
        // if is not leaf
        char* neighbor1;
        int neighbor1_indice;
        char* neighbor2;
        int neighbor2_indice;
        int is_neighbor1_ready = 0;

        // assign name to neighbor 1 and neighbor 2
        for (int i = 0; i < 3; i++){
            // printf("get temp. ");
            char *temp = (*((nodes+node_indice)->neighbors + i))->name;
            if (!compare(known_node_name, temp)){

                if (!is_neighbor1_ready){
                    neighbor1 = temp;
                    is_neighbor1_ready = 1;
                }
                else{
                    neighbor2 = temp;
                }
            }
        }
            // printf("neighbor 1 is %s", neighbor1);
            // printf("neighbor 2 is %s\n", neighbor2);
        // assign index to neighbor 1 and neighbor 2
        for (int i = 0 ; i < num_all_nodes; i++){
            char *temp = *(node_names + i);
            if (compare(temp, neighbor1)){
                neighbor1_indice = i;
            }
            else if (compare(temp, neighbor2)){
                neighbor2_indice = i;
            }
        }
        // first neighbor
        newick_process(neighbor1_indice, node_indice);
        edge = *(*(distances + node_indice) + neighbor1_indice);
        fprintf(stdout, "%s:%0.2f,", neighbor1, edge);

        // second neighbor
        newick_process(neighbor2_indice, node_indice);
        edge = *(*(distances + node_indice) + neighbor2_indice);
        fprintf(stdout, "%s:%0.2f)", neighbor2, edge);
    }
}

int emit_newick_format(FILE *out) {
    initialize_row_sums();
    int outlier_indice = -1;
    int parent_of_outlier_indice;
    char *parent_name;
    int return_value = 0;
    while (num_active_nodes > 2){
        int i = 0;
        int j = 0; //record the indice the pair two node has smallest Q
        //call function to compute min
        minimum_Q_Value(&i, &j);
        //set i <= j, avoid potential mistake
        if (j < i){
            int temp = i;
            i = j;
            j = temp;
        }
        //After find out the smallest Q value, we can insert new node. and Find the edge
        int indice_new_node = update_matrix_with_new_node(i,j);
        return_value = compute_edge_data(i, j, indice_new_node, out, 0);
        update_sum_rows(); //after get edge, update the rows_sum
    }

    //initialize row of sum
    // initialize row_sums to 0
    for (int i = 0; i < num_taxa; i++) {
        *(row_sums + i) = 0;
    }

    for (int i = 0; i < num_taxa; i++){
        for (int j = 0; j < num_taxa; j++){
            *(row_sums + i) += *(*(distances + i) + j);
        }
    }
    
    //if outlier name is not specified
    if (outlier_name == NULL){
        int max = 0;
        // pick the max row as default outlier
        for (int i = 0; i < num_taxa; i++){
            if (max < *(row_sums+i)){
                max = *(row_sums+i);
                outlier_name = *(node_names + i);
                outlier_indice = i;//this is real index for outlier
                parent_of_outlier_indice = parent_index(&parent_name, outlier_indice);
            }
        }
    }
    // if ouliter is specified 
    else{
        for (int i = 0 ;i < num_all_nodes; i++){
            if (compare(outlier_name, *(node_names+i))){
                //find outlier real index
                outlier_indice = i;
                parent_of_outlier_indice = parent_index(&parent_name, outlier_indice);
            }
        }
        // the specific outlier name not found
        if (outlier_indice == -1){
            fprintf(stderr, "The outlier is not found.\n");
            return -1;
        }
    }

    //recursion
    newick_process(parent_of_outlier_indice, outlier_indice);
    fprintf(stdout,"%s:%0.2f\n",parent_name,*(*(distances+parent_of_outlier_indice)+outlier_indice));

    return return_value;
}

/**
 * @brief  Emit the synthesized distance matrix as CSV.
 * @details  This function emits to a specified output stream a representation
 * of the synthesized distance matrix resulting from the neighbor joining
 * algorithm.  The output is in the same CSV form as the program input.
 * The number of rows and columns of the matrix is equal to the value
 * of num_all_nodes at the end of execution of the algorithm.
 * The submatrix that consists of the first num_leaves rows and columns
 * is identical to the matrix given as input.  The remaining rows and columns
 * contain estimated distances to internal nodes that were synthesized during
 * the execution of the algorithm.
 *
 * @param out  Stream to which to output a CSV representation of the
 * synthesized distance matrix.
 * @return 0 in case the output is successfully emitted, otherwise -1
 * if any error occurred.
 */
int emit_distance_matrix(FILE *out) {
    initialize_row_sums();
    int return_value = 0;
    while (num_active_nodes > 2){
        int i = 0;
        int j = 0; //record the indice the pair two node has smallest Q
        //call function to compute min
        minimum_Q_Value(&i, &j);
        //set i <= j, avoid potential mistake
        if (j < i){
            int temp = i;
            i = j;
            j = temp;
        }
        //After find out the smallest Q value, we can insert new node. and Find the edge
        int indice_new_node = update_matrix_with_new_node(i,j);
        return_value = compute_edge_data(i, j, indice_new_node, out, 0);
        update_sum_rows(); //after get edge, update the rows_sum
    }
    //print the node name in the first line
    for (int i = 0; i < num_all_nodes; i++){
        fprintf(stdout,",%s", *(node_names+i));
    }
    fprintf(stdout, "\n");

    //printf the entire matrix
    for (int i = 0; i < num_all_nodes; i++){
        fprintf(stdout, "%s,", *(node_names+i));
        for (int j = 0; j < num_all_nodes; j++){
            fprintf(stdout, "%0.2f ", *(*(distances+i)+j));
        }
        fprintf(stdout,"\n");
    }

    return return_value;
}

/**
 * @brief  Build a phylogenetic tree using the distance data read by
 * a prior successful invocation of read_distance_data().
 * @details  This function assumes that global variables and data
 * structures have been initialized by a prior successful call to
 * read_distance_data(), in accordance with the specification for
 * that function.  The "neighbor joining" method is used to reconstruct
 * phylogenetic tree from the distance data.  The resulting tree is
 * an unrooted binary tree having the N taxa from the original input
 * as its leaf nodes, and if (N > 2) having in addition N-2 synthesized
 * internal nodes, each of which is adjacent to exactly three other
 * nodes (leaf or internal) in the tree.  As each internal node is
 * synthesized, information about the edges connecting it to other
 * nodes is output.  Each line of output describes one edge and
 * consists of three comma-separated fields.  The first two fields
 * give the names of the nodes that are connected by the edge.
 * The third field gives the distance that has been estimated for
 * this edge by the neighbor-joining method.  After N-2 internal
 * nodes have been synthesized and 2*(N-2) corresponding edges have
 * been output, one final edge is output that connects the two
 * internal nodes that still have only two neighbors at the end of
 * the algorithm.  In the degenerate case of N=1 leaf, the tree
 * consists of a single leaf node and no edges are output.  In the
 * case of N=2 leaves, then no internal nodes are synthesized and
 * just one edge is output that connects the two leaves.
 *
 * Besides emitting edge data (unless it has been suppressed),
 * as the tree is built a representation of it is constructed using
 * the NODE structures in the nodes array.  By the time this function
 * returns, the "neighbors" array for each node will have been
 * initialized with pointers to the NODE structure(s) for each of
 * its adjacent nodes.  Entries with indices less than N correspond
 * to leaf nodes and for these only the neighbors[0] entry will be
 * non-NULL.  Entries with indices greater than or equal to N
 * correspond to internal nodes and each of these will have non-NULL
 * pointers in all three entries of its neighbors array.
 * In addition, the "name" field each NODE structure will contain a
 * pointer to the name of that node (which is stored in the corresponding
 * entry of the node_names array).
 *
 * @param out  If non-NULL, an output stream to which to emit the edge data.
 * If NULL, then no edge data is output.
 * @return 0 in case the output is successfully emitted, otherwise -1
 * if any error occurred.
 */
 void initialize_row_sums(){
    // initialize row_sums to 0
    for (int i = 0; i < num_taxa; i++) {
        *(row_sums + i) = 0;
    }

    int N = num_active_nodes;
    for (int i = 0; i < N; i++){
        int x = *(active_node_map + i);
        for (int j = 0; j < N; j++){
            int y = *(active_node_map + j);
            *(row_sums + x) += *(*(distances + x) + y);
        }
    }
 }

 int update_matrix_with_new_node(int i, int j){
    //assign a new name to new node
    *(*(node_names + num_all_nodes)) = '#'; 
    int temp = num_all_nodes;
    int new_node_indice;
    int num_digit = 0;
    int tempCopy = temp;
    
    while (tempCopy > 0) {
        num_digit++;
        tempCopy /= 10;
    }
    int x = 0;
    while (temp > 0) {
        int digit = temp % 10; 
        *(*(node_names + num_all_nodes) + num_digit) = '0' + digit; 
        temp /= 10; 
        num_digit--;
        x++;
    }
    *(*(node_names + num_all_nodes) + x+1) = '\0'; //null terminator in the end

    //update new matrix with new node 
    //initialize the new comming node difference value
    for (int a = 0; a < num_all_nodes; a++){
        *(*(distances+num_all_nodes)+a) = 0;
        *(*(distances+a)+num_all_nodes) = 0;
    }

    //fix position of i, it shouldn't in the end of active map
    if (i == *(active_node_map + num_active_nodes-1 )){
        int temp = i;
        i = j;
        j = i;
    }
    //active_node_map[i] = num_all_nodes;
    for (int m = 0; m < num_active_nodes; m++){
        if (i == *(active_node_map + m)){
            *(active_node_map + m) = num_all_nodes;
            new_node_indice = *(active_node_map + m);
        }
        if (j == *(active_node_map + m))
            *(active_node_map + m) = *(active_node_map + num_active_nodes - 1);
    }
    
    *(*(node_names + num_all_nodes)) = '#';
    int temp_index_name = 1;
    int temp_indice = new_node_indice;
    while (temp_indice > 0){
        int digit = temp_indice % 10;
        *(*(node_names + num_all_nodes) + temp_index_name) = digit + '0';
        temp_indice /= 10;
    }
    (nodes + num_all_nodes)->name = *(node_names + num_all_nodes);
    num_active_nodes--;//active node decrement
    num_all_nodes++; //num of node increment

    //update row of sum & update distance matrix
    for (int a = 0; a < num_active_nodes; a++){
        int indice = *(active_node_map + a);
        *(*(distances+num_all_nodes-1)+indice) = 1.0/2.0*(*(*(distances+i)+indice) + *(*(distances+j)+indice)-*(*(distances+i)+j));
        *(*(distances+indice)+num_all_nodes-1) = *(*(distances+num_all_nodes-1)+indice);
        if (indice == num_all_nodes-1){
            *(*(distances+num_all_nodes-1)+indice) = 0;
            *(*(distances+indice)+num_all_nodes-1) = 0;
        }
    }
    return new_node_indice;
}

void update_sum_rows(){
    for (int a = 0; a < num_active_nodes; a++){
        int indice = *(active_node_map + a);
        *(row_sums + indice) = 0; //set to 0 for those are going update cell
        for (int b = 0; b < num_active_nodes; b++){
            int indice2 = *(active_node_map + b);
            //*(row_sums + i) += *(*(distances + i) + j);
            *(row_sums + indice) += *(*(distances + indice) + indice2);
        }
    }
}

int computeQ(int i, int j){
    int N = num_active_nodes;
    int ans = 0;
    int row_sum_i = 0;
    int row_sum_j = 0;
    //Compute the sigma k = 1: d(i,k) and Compute the sigma k = 1: d(j,k)
    for (int a = 0; a < N; a++){
        //int x = active_node_map[a];
        int x = *(active_node_map + a);
        //row_sum_i += distances[i][a];
        row_sum_i += *(*(distances+i)+x);
        //row_sum_j += distances[j][a];
        row_sum_j += *(*(distances+j)+x);
    }
    //ans = (N-2)* distances[i][j] - row_sum_i - row_sum_i;
    ans = (N-2) * *(*(distances+i)+j) - row_sum_i - row_sum_j;
    return ans;
}

int minimum_Q_Value(int* i, int* j){
    int Q = 2147483647; //current min Q value
    int N = num_active_nodes; // number of node is in consideration
    int temp1, temp2; 
    for (int a = 0; a < N-1; a++){
        // *i = active_node_map[a];
        temp1 = *(active_node_map + a);
        for (int b = a+1; b < N; b++){
            // *j = active_node_map[b];
            temp2 = *(active_node_map+b);
            int tempQ = computeQ(temp1,temp2);
            if (Q > tempQ){
                Q = tempQ;
                *i = temp1;
                *j = temp2;
            }
        }
    }
    return Q;
}

 void add_nodes_neighbors(int i, int j){
    for (int index_neighbor = 0; index_neighbor < 3; index_neighbor++){
        if (*((nodes + i)->neighbors+index_neighbor) == NULL){
            *((nodes + i)->neighbors+index_neighbor) = nodes+j;
            break;
        }
    }
    for (int index_neighbor = 0; index_neighbor < 3; index_neighbor++){
        if (*((nodes + j)->neighbors+index_neighbor) == NULL){
            *((nodes + j)->neighbors+index_neighbor) = nodes+i;
            break;
        }
    }
 }
 void add_parent_neighbors(int i, int parent){
    for (int index_neighbor = 1; index_neighbor < 3; index_neighbor++){
        if (*((nodes + parent)->neighbors+index_neighbor) == NULL){
            *((nodes + parent)->neighbors+index_neighbor) = nodes+i;
            break;
        }
    }
    // first index if the parent of current node
    *((nodes + i)->neighbors) = nodes + parent;
 }

int compute_edge_data(int i, int j, int indice_new_node, FILE* out, int printable){
    int N = num_active_nodes;
    double edge_i_and_new_node = (1.0/2.0 * (*(*(distances + i) + j))) + 1.0/(2.0*(N+1.0-2.0)) * (*(row_sums + i) - *(row_sums + j));
    double edge_j_and_new_node = (*(*(distances + i) + j)) - edge_i_and_new_node;

    // i and j neighbor to new node
    add_parent_neighbors(i, indice_new_node);
    add_parent_neighbors(j, indice_new_node);

    //update matrix with new edge for 
    *(*(distances + i) + indice_new_node) = edge_i_and_new_node;
    *(*(distances + indice_new_node) + i) = edge_i_and_new_node;
    //update matrix with new edge for j
    *(*(distances + j) + indice_new_node) = edge_j_and_new_node;
    *(*(distances + indice_new_node) + j) = edge_j_and_new_node;

    if (printable == 1){
        fprintf(stdout,"%d,%d,%0.2f\n%d,%d,%0.2f\n",i,indice_new_node,edge_i_and_new_node,j,indice_new_node,edge_j_and_new_node);
    }

    if (N <= 2){
        for (int a = 0; a < num_active_nodes; a++){
            if (indice_new_node != *(active_node_map+a)){
                j = *(active_node_map+a); //j is the last leaf indice
            }
        }
        double last_nodes_edge =  *(*(distances+i)+j) - edge_i_and_new_node;
        //update matrix with new edge for last leaf and new node
        *(*(distances + j) + indice_new_node) = last_nodes_edge;
        *(*(distances + indice_new_node) + j) = last_nodes_edge;
         
        // add neighbor between new node and previous new node
        for (int a = 0; a < num_active_nodes; a++){
            int last_node = *(active_node_map+a);
            //find out the internal node that don't have 3 neighbors
            if (last_node != indice_new_node){ 
                add_nodes_neighbors(indice_new_node, last_node);
                break;
            }
        }
        if (printable == 1)
            fprintf(stdout, "%d,%d,%0.2f\n",j,indice_new_node,last_nodes_edge);
        num_active_nodes = 0;

    }
    //update matrix with new edge

    return 0;
}


int build_taxonomy(FILE *out) {
    initialize_row_sums();

    int return_value = 0;
    while (num_active_nodes > 2){
        int i = 0;
        int j = 0; //record the indice the pair two node has smallest Q
        //call function to compute min
        minimum_Q_Value(&i, &j);
        //set i <= j, avoid potential mistake
        if (j < i){
            int temp = i;
            i = j;
            j = temp;
        }
        //After find out the smallest Q value, we can insert new node. and Find the edge
        int indice_new_node = update_matrix_with_new_node(i,j);
        return_value = compute_edge_data(i, j, indice_new_node, out, 1);
        update_sum_rows(); //after get edge, update the rows_sum
    }

    return return_value;
}
