/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"
#include "errno.h"

//Functino declaration
int get_proper_alignment(size_t size);
size_t get_block_size(sf_header header);
size_t* get_index_size_of_freeList();
void insert_wildness(sf_block* block);
int get_prev_alloc(sf_block* block);
void initialize_sf_free_list_heads();
sf_block* coalesce(sf_block* block);
void set_block_size(sf_block* block, size_t size, int is_epilogue);
void set_payload_size(sf_block* block, size_t size, int is_epilogue);
void set_alloc(sf_block* block, int alloc, int is_epilogue);
int get_alloc(sf_header header);
sf_block* splitting(sf_block* block, size_t need_size, int end);
void inset_list_header(sf_block* block);

static size_t overall_payload_size = 0;
static size_t overall_block_size = 0;
static size_t overall_max_payload = 0;

//initialize the free block list·
void initialize_sf_free_list_heads(){
    for (int i = 0; i < NUM_FREE_LISTS; i++){
        sf_block* dummy = sf_free_list_heads+i;
        dummy->body.links.next = dummy;
        dummy->body.links.prev = dummy;
    }
}

// get the  payload size and padding in proper alignment
int get_proper_alignment(size_t size){
    int remaining = size%16;
    //remaining is not 0, then it is not proper aligned
    int is_aligned;
    if (remaining == 0)
        is_aligned = 1;
    else
        is_aligned = 0;

    if (!is_aligned){
        int temp = size/16;
        temp++;
        size = temp * 16;
    }
    return size;
}

// get the blsock_size from header or footer
size_t get_block_size(size_t header){
    size_t ans = 0;
    //mask off payload_size and lowerest 4 bits
    size_t mask = 0xFFFFFFF0;
    ans = header & mask;
    return ans;
}

//get the payload size from hedaer or footer
size_t get_payload_size(size_t header){
    size_t ans = 0;
    //mask off block_size bits (32 bits in LSD)
    ans = header >> 32;
    return ans;
}

//get index_map for free blocks lists
size_t* get_index_size_of_freeList(){
    static size_t list_index_size[NUM_FREE_LISTS-1];
    list_index_size[0] = 1 * 32;
    list_index_size[1] = 2 * 32;
    for (int i = 2; i < NUM_FREE_LISTS-2; i++){
        list_index_size[i] = list_index_size[i-1] + list_index_size[i-2];
    }
    list_index_size[NUM_FREE_LISTS-1] = 0;
    return list_index_size;
}


//update block_size value for specific block
void set_block_size(sf_block* block, size_t size, int is_epilogue){
    // get current header
    sf_header* header = (sf_header*) &(block->header);
    // mask for clear bit 4 to 32
    size_t mask = 0xfffffff0;
    *header = *header & (~mask);
    // update new size for it in 4 to 32
    *header = *header | size;
    //update footer
    if(!is_epilogue){
        // get access to footer, 8 is the header size, "size" is the entire block
        sf_footer* footer = (sf_footer*) ((char*)block + size);
        *footer = *header;
    }
}

//update payload_size value for specific block
void set_payload_size(sf_block* block, size_t size , int is_epilogue){
    // get current header
    sf_header* header = &(block->header);
    // mask to clear 32 to 64
    size_t mask = 0xffffffff;
    *header = *header & mask;
    // uodate new size for it in 32 to 64
    *header = *header | (size << 32);
    // update footer
    // get access to footer, 8 is the header size, "size" is the payload
    size = get_proper_alignment(size);
    size_t block_size = get_block_size(block->header);

    if(!is_epilogue){
        sf_footer* footer = (sf_footer*) ((char*)block+block_size);
        *footer = *header;

    }
}

//update alloc status and prev_alloc
void set_alloc(sf_block* block, int alloc, int is_epilogue){
    // get current header
    sf_header* header = &(block->header);
    //mask off lowerest 3rd and 4nd bit, 3 is bit in ..0011
    size_t mask = 15; // 0001111
    *header = *header & ~(mask); //~mask is ..11110000
    // get prev_alloc
    int prev_alloc = get_prev_alloc(block);

    prev_alloc = prev_alloc << 2; //move to correct bit position
    // combine the alloc and prev_alloc
    int current_block_alloc = (alloc << 3) | prev_alloc;
    //update current block header 
    *header = *header | current_block_alloc;

    if (!is_epilogue){
        //update current block footer
        // size_t current_block_size = get_block_size(block->header);
        // sf_footer* footer = (sf_footer*)((char*)block + current_block_size);
        // *footer = *header;

        // update footer
        size_t block_size = get_block_size(block->header);
        sf_footer* footer = (sf_footer*) ((char*)header+block_size-8);
        *footer = *header;

        //update next block's header prev_alloc bit
        sf_block* next_block = (sf_block*) footer;
        sf_header* next_header = &(next_block->header);
        //get alloc bit for current Block, prev alloc for next block
        int next_block_alloc = alloc << 2;
        // this will be pre_alloc for next block
        size_t next_block_size = get_block_size(next_block->header);
        //size_t next_block_payload_size = get_payload_size(next_block->header);
        //clean the prev-alloc bit first
        *next_header = *next_header & ~(1 << 2);
        *next_header = *next_header | next_block_alloc;
        sf_footer* next_block_footer;
        if (next_block_size == 0){
            next_block_footer = (sf_footer*) ((char*)next_block + 8);
        }
        else{
            next_block_footer = (sf_footer*) ((char*)next_block + next_block_size);
        }

        *next_block_footer = *next_header;

    }
}

// get the status of allocation or not in header or footer
int get_alloc(sf_header header){
    //discard the prev alloc 1 bit and unused 2 bits in the lowerest 3 bits
    header = header >> 3;
    // 1 is 00000000...1 which can mask off all other than lowerest bit;
    int alloc = header & 1;
    return alloc;
}

// get previous alloc or not
int get_prev_alloc(sf_block* block){
    //get previous footer
    sf_footer prev_footer = block->prev_footer;
    //discard the unused 2 bits in the lowerest 2 bits and 1 bit in the 3rd bit
    prev_footer = prev_footer >> 3;
    // 1 is 000.1 which can mask off all other than lowerest bit
    int prev_alloc = prev_footer & 1;
    return prev_alloc;
}

//check prev block, if it is not alloc. then coalesce
//return* the block after coalescing
sf_block* coalesce(sf_block* block){
    sf_footer* prev_footer = &(block->prev_footer);
    int prev_alloc = get_alloc(*prev_footer);
    //get the poinrter of next block
    size_t current_block_size = get_block_size(block->header);
    sf_block* next_block = (sf_block*) ((char*) block + current_block_size);
    //we need to check whether next_block is in wilderness or not
    sf_block* dummy = sf_free_list_heads+9;
    //if next_block is in wildness, we should put it back to wildnerss after combining
    int bool_wilderness = 0;
    if (dummy->body.links.next == next_block){
        bool_wilderness = 1;
    }
    int next_alloc = get_alloc(next_block->header);

    //if next block is unallocated, combine current and next block 
    if(!next_alloc){
        current_block_size += get_block_size(next_block->header);
        //set current block size to total size (add current and next block)
        set_block_size(block, current_block_size, 0);
        set_payload_size(block, 0, 0);
        set_alloc(block, 0, 0);
        //remove the next_block links since it was combined with previous 
        sf_block* prev = next_block->body.links.prev;
        sf_block* next = next_block->body.links.next;
        if (prev != NULL && next != NULL){
            //set next and prev to NULL since this block is not existed anymore
            next_block->body.links.prev = NULL;
            next_block->body.links.next = NULL;
            //links the original prev's next to next, next's prev to prev
            prev->body.links.next = next;
            next->body.links.prev = prev;
        }
    }
    // if previous block is not allocated, which is free, then combine with current block
    if (!prev_alloc){
        //size_t prev_block_size = get_block_size(block);
        size_t prev_block_size = get_block_size(*prev_footer);
        size_t total_size = prev_block_size + current_block_size;
        // move the pointer of block footer back block_size, and + 8, we get block header
        sf_block* prev_block = (sf_block*) ((char*)prev_footer - prev_block_size);
        set_block_size(prev_block, total_size,0);
        set_payload_size(prev_block, 0, 0);
        set_alloc(prev_block, 0, 0);

        //remove the block links since it was comibned and no existed
        sf_block* prev = block->body.links.prev;
        sf_block* next = block->body.links.next;
        //set block link to null
        if (prev != NULL && next != NULL){
            block->body.links.prev = NULL;
            block->body.links.next = NULL;
            //set old prev'next to next, old next' prev link to prev
            prev->body.links.next = next;
            next->body.links.prev = prev;
        }

        //assign current block pointer to the new combined block
        block = prev_block;
    }

    //put combined block into correct list
    if (block->body.links.prev != NULL && block->body.links.next){
        sf_block* prev = block->body.links.prev;
        sf_block* next = block->body.links.next;
        prev->body.links.next = next;
        next->body.links.prev = prev;
    }

    //insert into block heads
    if(bool_wilderness == 1){
        insert_wildness(block);
    }
    else{
        inset_list_header(block);
    }

    return block;
}

// if the block_size is enough big for need_size, splitting it if there is no splinter
// return* block size that was splitted and allocated
sf_block* splitting(sf_block* block, size_t need_size, int end){
    // get the block of remainning memory
    size_t size = get_block_size(block->header);
    size_t remaining_size = size - need_size;

    //remove current block from list heads
    if (block->body.links.next != NULL && block->body.links.prev != NULL){
        sf_block* prev = block->body.links.prev;
        block->body.links.prev = NULL;
        sf_block* next = block->body.links.next;
        block->body.links.next = NULL;
        prev->body.links.next = next;
        next->body.links.prev = prev;
    }

    //if the remaining mem size < 32, which would cause a splinter, so do not splitting
    if (remaining_size < 32){
        return block;
    }
    else{
        sf_block* remaining_block = (sf_block*) ((char*) block + need_size);
        set_block_size(remaining_block, remaining_size, 0);
        set_payload_size(remaining_block, 0, 0);
        set_alloc(remaining_block, 0, 0);

        //put the remaining block in to the free lists heads
        if (!end){
            inset_list_header(remaining_block);
        }
        else{
            //insert into wilderness
            sf_block* dummy = sf_free_list_heads+9;
            dummy->body.links.next = remaining_block;
            remaining_block->body.links.prev = dummy;
            dummy->body.links.prev = remaining_block;
            remaining_block->body.links.next = dummy;
        }
        // coalesce(remaining_block);


        sf_header* header = &(remaining_block -> header);
        sf_footer* footer = (sf_footer*)((char*)remaining_block+remaining_size);
        *footer = *header;

        return remaining_block;
    }
}

//insert memory into wilderness lists
void insert_wildness(sf_block* block){
    //insert into wilderness
    sf_block* dummy = sf_free_list_heads+9;
    dummy->body.links.next = block;
    block->body.links.prev = dummy;
    dummy->body.links.prev = block;
    block->body.links.next = dummy;
}

//insert the block into the free list header
void inset_list_header(sf_block* block){
    size_t size = get_block_size(block->header);
    size_t* list_index_size = get_index_size_of_freeList();
    for (int i = 0; i < NUM_FREE_LISTS-1; i++){
        if (list_index_size[i] >= size){
            // get the head pointer
            sf_block* dummy = sf_free_list_heads+i;
            //get the dummy next 
            sf_block* temp = dummy->body.links.next;
            //set the dummy next is the new comming block, since it is circular
            dummy->body.links.next = block;
            block->body.links.prev = dummy;
            //set the block prev to the original prev
            block->body.links.next = temp;
            temp->body.links.prev = block;
            break;
        }
    }

}

void *sf_malloc(size_t size) {

    // To be implemented.
    size_t need_size = (get_proper_alignment(size) + 16);

    //setup global variable
    overall_payload_size += size;
    overall_block_size += need_size;

    if (overall_payload_size > overall_max_payload){
        overall_max_payload = overall_payload_size;
    }

    // if this is first implementation, and there is no memory in heap
    if (sf_mem_start() == sf_mem_end()){
        // initialize free blocks lists array
        initialize_sf_free_list_heads();

        // set up prologue
        sf_block* prologue = (sf_block*) sf_mem_grow();
        //if it return NULL, means it error, not more memory
        if (prologue == NULL){
            sf_errno = ENOMEM;
            return NULL;
        }
        //initialize the block
        //initialize_block(prologue);

        //initialize payload_size to 0;
        set_block_size(prologue, 32, 0);
        set_payload_size(prologue, 0, 0);

        //initialize for prologue, right most 4 is not used, which should be 1000 which is in 8
        int mask = 8;
        // update alloc 1 bit , unused 1 bit , unused 2 bit
        prologue->header = (prologue->header) | mask;
        //get access to prologue's footer
        sf_footer* prologue_footer = (sf_footer*) ((char*) prologue + 32);
        //make the footer identical with header
        *prologue_footer = prologue->header;
        // the new size get
        size_t new_size = PAGE_SZ;
        // deduct the size of prologue which is 32 bytes and 8 bytes for padding
        new_size -= 40;
        overall_block_size += 40;
        // deduct the size for the epilogue, which is 8 bytes only header
        new_size -= 8;
        overall_block_size += 8;

        // get next available memory space, initialize new block
        sf_block* block = (sf_block*)((char*)prologue + 32);
        //initialize_block(block);

        // need to make sure the memo is enough for the block, which payload, padding and header footer
        while(new_size < need_size){
            //if memory can't satisfy this requested memory
            if(sf_mem_grow() == NULL){
                insert_wildness(block);
                sf_errno = ENOMEM;
                return NULL;
            }
            new_size += PAGE_SZ;
            set_block_size(block, new_size, 0);
            set_payload_size(block, 0, 0);
            set_alloc(block, 0, 0);
        }
        //block is get grow
        set_block_size(block, new_size, 0);
        set_payload_size(block, 0, 0);

        //split the block
        splitting(block, need_size, 1);
        set_block_size(block, need_size, 0);
        set_payload_size(block, size, 0);
        set_alloc(block,1, 0);
        

        // set up epilogue
        sf_block* epilogue = (sf_block*) ((char*)sf_mem_end() - 16);
        //initialize_block(epilogue);
        // intialize payload_size to 0, and set block_size to 0, alloc is 1, 
        epilogue->header = 0;
        //update alloc and prev_alloc, alloc should be 1
        set_block_size(epilogue, 8, 1);
        set_payload_size(epilogue, 0, 1);
        set_alloc(epilogue, 1, 1);

        //return the payload area to caller, 8 bytes is the size of header, after 8 bytes is payload area   
        return &(block->body.payload);
    }

    // if the request size is 0 , return null without setting errno
    if (size <= 0)  return NULL;
    //the minimum of block size is 32, never less than 32 bytes
    //if (size < 32) size = 32;

    // this can get size block that each index contain in free block list
    size_t* list_index_size = get_index_size_of_freeList();

    //the start index to search free block list
    int start_index = 0;
    //loop to find proper start index
    for (int i = 0; i < NUM_FREE_LISTS-1; i++){
        if (size >= list_index_size[i]){
            //once found, set start_index and break;
            start_index = i;
        }

    }
     // search through the free lists
    int need_mem_grow = 1; //check whether need to call sf_mem_grow
    sf_block* current; // this is the cursor of node, to keep next to find proper block
    for (int i = start_index; i < NUM_FREE_LISTS-1; i++){
        //dummy is this beginning of the node, link with last and first block
        sf_block* dummy = sf_free_list_heads+i;

        //this is the cursor of node, to keep next to find proper block
        current = dummy->body.links.next;
        //check whether current node back to dummy
        while (dummy != current){
            //extract the block size from header
            size_t block_size = get_block_size(current->header);
            if (block_size < need_size){
                // if current block size is no enough, check next block in blocks list
                current = current->body.links.next;
            }
            else{
                //find block found, we dont need to grown mem
                need_mem_grow = 0;
                //split current block
                splitting(current, need_size, 0);
                //update current block header and footer
                set_block_size(current, need_size, 0);
                set_payload_size(current, size, 0);
                set_alloc(current, 1, 0);
                break;
            }
        }
    }
    //if true, which means it dones't find proper block from list head 0 to 8
    //now we search wilderness
    if (need_mem_grow){
        sf_block* dummy = sf_free_list_heads+9;
        //int wilderness_empty = 0;
        if(dummy->body.links.next != NULL && dummy->body.links.prev != NULL){
            current = dummy->body.links.next;
            if (current != dummy){
                size_t current_size = get_block_size(current->header);
                if (current_size >= need_size){
                    //find block
                    need_mem_grow = 0;
                    //split current block
                    splitting (current, need_size, 1);
                    set_block_size(current, need_size, 0);
                    set_payload_size(current, size, 0);
                    set_alloc(current, 1, 0);

                }
            }
        }
    }
    // if need_mem_grow is 1. which means we didn't find proper blocks
    if (need_mem_grow){
        // get header for the new page after mem_grow
        sf_block* new_page = sf_mem_grow();
        //if no more memory
        if(new_page == NULL){
            sf_errno = ENOMEM;
            return NULL;
        }
        new_page = (sf_block*)((char*) new_page - 16);
        size_t new_page_size = PAGE_SZ;
        //since we need 8 bytes for epilogue
        // new_page_size -= 8;
        //set header and footer value  
        // block_size is 16 byte (header and footer) + payload size
        set_block_size(new_page, new_page_size, 0);
        set_payload_size(new_page, 0, 0);
        set_alloc(new_page, 0, 0);
        //check whether previous block alloc, if not, then coalesce
        current = (sf_block*) coalesce(new_page);
        insert_wildness(current);
        size_t current_size = get_block_size(current->header);
        //if there are not enough mem, continue grow, and add new size in to the current block
        while (current_size < need_size){
            new_page = sf_mem_grow();
            //if sf_mem_grow return NULL, which mean there no more mem, insert the last block in to wilderness and return NULL
            if(sf_mem_grow() == NULL){
                insert_wildness(current);
                sf_errno = ENOMEM;
                return NULL;
            }
            current_size += PAGE_SZ;
            set_block_size(current, current_size, 0);
            set_payload_size(current, 0, 0);
            set_alloc(current, 0, 0);
        }
        // splitting current block, and insert the rest of memory in to wirlderness
        splitting(current, need_size, 1);
        set_block_size(current, need_size, 0);
        set_payload_size(current, size, 0);
        set_alloc(current,1, 0);
        //     sf_header* HEADER = (sf_header*)((char*)sf_mem_start()+40);
        // printf("HEADER POINTER %p\n", HEADER);
        // printf("HEDAER %lu\n", *HEADER);
        // sf_footer* FOOTER =(sf_footer*)((char*)HEADER+4048-8);
        // printf("FOOTER POINTER %p\n", FOOTER);
        // printf("FOOTER %lu\n\n", *FOOTER);
        //setup epilogue
        sf_block* epilogue = (sf_block*) ((char*)sf_mem_end() - 16);
        epilogue->header = 0;
        set_block_size(epilogue, 8, 1);
        set_payload_size(epilogue,0,1);
        set_alloc(epilogue, 1, 1);
        //printf("epilogue %p\n", epilogue);

    }


    //check wether the free block in wilderness has size 0
    // sf_block* last_free_block = (sf_free_list_heads+9)->body.links.next;
    // size_t last_free_block_size = get_block_size(last_free_block->header);
    // if(last_free_block_size == 0){
    //     sf_block* temp = last_free_block->body.links.prev;
    //     sf_block* temp2 = last_free_block->body.links.next;
    //     last_free_block->body.links.prev = NULL;
    //     last_free_block->body.links.next = NULL;
    //     temp->body.links.next = temp2;
    //     temp2->body.links.prev = temp;
    //     last_free_block->header = 0;
    //     last_free_block->prev_footer = 0;
    // }
    sf_block* epilogue = (sf_block*) ((char*)sf_mem_end() - 16);
    epilogue->header = 0;
    set_block_size(epilogue, 8, 1);
    set_payload_size(epilogue,0,1);
    set_alloc(epilogue, 1, 1);

    //return the payload area
    return &(current->body.payload);
}


void sf_free(void *pp) {
    // To be implemented.
    //invalid pointer
    if(pp == NULL){
        abort();
    }
    //get current block pointer
    sf_block* current = (sf_block*)((char*)pp - 16);
    int alloc = get_alloc(current->header);

    //update global variable
    size_t block_s = get_block_size(current->header);
    size_t payload_s = get_payload_size(current->header);
    overall_payload_size -= payload_s;
    overall_block_size -= block_s;

    //if it is unallocated block
    if(!alloc){
        abort();
    }
    set_payload_size(current, 0, 0);
    set_alloc(current, 0, 0);
    coalesce(current);
}

/*
 * Marks a dynamically allocated region as no longer in use.
 * Adds the newly freed block to the free list.
 *
 * @param ptr Address of memory returned by the function sf_malloc.
 *
 * If ptr is invalid, the function calls abort() to exit the program.
 */
void *sf_realloc(void *pp, size_t rsize) {
    //invalid pointer
    if(pp == NULL){
        abort();
    }

    //get current block payload pointer
    sf_block* current = (sf_block*)((char*)pp - 16);
    void* current_payload = pp;
    size_t current_size = get_payload_size(current->header);

    if (current_size > rsize){
        //if it lead a splinter. then don't split
        if (current_size - rsize < 32){
            return pp;
        }

        //update global variable
        size_t payload_s = get_payload_size(current->header);
        size_t block_s = get_block_size(current->header);
        overall_payload_size -= payload_s;
        overall_block_size -= block_s;

        size_t aligned = get_proper_alignment(rsize)+16;
        sf_block* remaining_block = splitting(current, aligned, 0);
        set_block_size(current, aligned, 0);
        set_payload_size(current, rsize, 0);
        set_alloc(current, 1, 0);
        coalesce(remaining_block);

        //update global variable
        overall_payload_size += rsize;
        overall_block_size += aligned;

        return current_payload;
    }
    else if (current_size == rsize){
        return pp;
    }
    else{
        //update global variable
        size_t payload_s = get_payload_size(current->header);
        size_t block_s = get_block_size(current->header);
        overall_payload_size -= payload_s;
        overall_block_size -= block_s;

        //get new block payload pointer
        void* new_block_payload = sf_malloc(rsize);

        //copy the memory from current block to the new block
        memcpy(new_block_payload, current_payload, current_size);
        sf_free(current_payload);

        //update global variable
        sf_block* new_block = (sf_block*)((char*)new_block_payload-16);
        payload_s = get_payload_size(new_block->header);
        block_s = get_block_size(new_block->header);
        overall_payload_size += payload_s;
        overall_block_size += block_s;
        if(overall_payload_size > overall_max_payload){
            overall_max_payload = overall_payload_size;
        }

        return new_block_payload;
    }
}
/*
 * Get the current amount of internal fragmentation of the heap.
 *
 * @return  the current amount of internal fragmentation, defined to be the
 * ratio of the total amount of payload to the total size of allocated blocks.
 * If there are no allocated blocks, then the returned value should be 0.0.
 */
double sf_fragmentation() {
    // To be implemented.

    if (overall_block_size == 0){
        return 0.0;
    }
    //printf("%lf\n", (double)overall_payload_size/(double)overall_block_size);
    return  ((double)overall_payload_size/(double)overall_block_size);
}

/*
 * Get the peak memory utilization for the heap.
 *
 * @return  the peak memory utilization over the interval starting from the
 * time the heap was initialized, up to the current time.  The peak memory
 * utilization at a given time, as defined in the lecture and textbook,
 * is the ratio of the maximum aggregate payload up to that time, divided
 * by the current heap size.  If the heap has not yet been initialized,
 * this function should return 0.0.
 */
double sf_utilization() {
    if (sf_mem_start() == sf_mem_end()){
        return 0.0;
    }
    //max payload divided by current heap size
    double heap_size = sf_mem_end() - sf_mem_start();
    return (double)((double)overall_max_payload/heap_size);
    // To be implemented.
}
