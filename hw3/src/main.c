#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {
     //sf_malloc(4032);
    // sf_block* block = (sf_block*)((char*)x-16);
    // printf("header %lu\n", block->header);
    // sf_footer* footer = (sf_footer*) ((char*)block+4032+16);
    // printf("footer %lu\n", *footer);
    sf_malloc(4032);
    // sf_show_heap();
    void* x = sf_malloc(200);

    //sf_malloc(32);
   //sf_show_heap();
   void* y =sf_malloc(4000);
   sf_free(x);
   sf_malloc(10);
   sf_malloc(8);
   sf_free(y);
   sf_show_heap();

     // sf_free(y);

    return EXIT_SUCCESS;
}
