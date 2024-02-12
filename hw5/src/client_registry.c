#include "client_registry.h"
#include "pthread.h"
#include <semaphore.h>
#include <stdlib.h>
#include <sys/select.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>

#include <debug.h>

/*
 * A client registry keeps track of the file descriptors for clients
 * that are currently connected.  Each time a client connects,
 * its file descriptor is added to the registry.  When the thread servicing
 * a client is about to terminate, it removes the file descriptor from
 * the registry.  The client registry also provides a function for shutting
 * down all client connections and a function that can be called by a thread
 * that wishes to wait for the client count to drop to zero.  Such a function
 * is useful, for example, in order to achieve clean termination:
 * when termination is desired, the "main" thread will shut down all client
 * connections and then wait for the set of registered file descriptors to
 * become empty before exiting the program.
 */

typedef struct client {
    int fd;
    struct client* next;
} CLIENT;

typedef struct client_registry {
    // total number of client which connected with server
    int num_client;
    sem_t mutex; // Semaphore that protects num_client;
    sem_t num_client_mutex; // it is necessary for wait empty
    CLIENT* clients; //linked list to store all client
} CLIENT_REGISTRY;


/*
 * Initialize a new client registry.
 *
 * @return  the newly initialized client registry, or NULL if initialization
* fails.   
 */
CLIENT_REGISTRY *creg_init(){
    CLIENT_REGISTRY* registry = malloc(sizeof(CLIENT_REGISTRY));
    sem_init(&(registry->mutex), 0, 1); // mutex = 1
    sem_init(&(registry->num_client_mutex), 0, 1);// for check emtpy mutex
    registry->num_client = 0; // initialize num_client
    // registry->prev = NULL; // since this is the root, prev is always NULL
    registry->clients = NULL;
    return registry;
}

/*
 * Finalize a client registry, freeing all associated resources.
 * This method should not be called unless there are no currently
 * registered clients.
 *
 * @param cr  The client registry to be finalized, which must not
 * be referenced again.
 */
void creg_fini(CLIENT_REGISTRY *cr){
    sem_wait(&(cr->mutex));
    CLIENT* cursor = cr->clients;

    // closing all fd and free pointer
    while (cursor != NULL){ 
        // close file desciriptor
        close(cursor->fd);
        CLIENT* temp = cursor;
        cursor = cursor->next; //keep trace
        free(temp); // free current stack depth client
    }

    //once all client register freed
    // free client registry
    sem_post(&(cr->mutex));
    sem_destroy(&(cr->mutex));
    free(cr);
}

/*
 * Register a client file descriptor.
 *
 * @param cr  The client registry.
 * @param fd  The file descriptor to be registered.
 * @return 0 if registration is successful, otherwise -1.
 */
int creg_register(CLIENT_REGISTRY *cr, int fd){
    //block registry
    sem_wait(&(cr->mutex));

    // Iterate to find the next null ptr for new client 
    CLIENT* cursor = cr->clients;
    CLIENT* prev = NULL;
    while(cursor != NULL){
        prev = cursor;
        cursor = cursor->next;
    }
    cursor = prev;
    // once find a new null ptr, assign the new fd
    CLIENT* new_client = malloc(sizeof(CLIENT));
    if (new_client == NULL){
        sem_post(&(cr->mutex)); //Release the lock 
        return -1;
    }

    //first client connected
    if (cursor == NULL && prev == NULL){
        cr->clients = new_client;
    }
    else{
        cursor->next = new_client;
    }
    //assign value for the new client
    new_client->fd = fd;
    new_client->next = NULL;

    if (cr->num_client == 0){
        //num of client will from empty to 1, block num_client_mutex;
        sem_wait(&(cr->num_client_mutex));
    }
    //increment the number of client
    (cr->num_client)++;

    // release registry
    sem_post(&(cr->mutex));
    return 0;
}

/*
 * Unregister a client file descriptor, removing it from the registry.
 * If the number of registered clients is now zero, then any threads that
 * are blocked in creg_wait_for_empty() waiting for this situation to occur
 * are allowed to proceed.  It is an error if the CLIENT is not currently
 * registered when this function is called.
 *
 * @param cr  The client registry.
 * @param fd  The file descriptor to be unregistered.
 * @return 0  if unregistration succeeds, otherwise -1.
 */
int creg_unregister(CLIENT_REGISTRY *cr, int fd){
    //block registry
    sem_wait(&(cr->mutex));

    int count = 0;
    // Iterate to find the fd that want to unregister
    CLIENT* cursor = cr->clients;
    CLIENT* prev = NULL; //store previsou client from cursor
    while(cursor != NULL){
        if (cursor->fd == fd){
            close(cursor->fd); // close fd
            break;
        }
        // it should be less than FD_SETSIZE, if count > FD_SETSIZE
        // CLIENT is not currently registered
        if (count > FD_SETSIZE){
            sem_post(&(cr->mutex));
            return -1;
        }
        prev = cursor;
        cursor = cursor->next;
        count++;
    }

    //remove from the linked list in client registry
    if (prev != NULL && cursor != NULL){
        prev->next = cursor->next;
    }
    else if (prev == NULL && cursor != NULL){
        // which cursor is this first client in registory
        cr->clients = cursor->next;
    }
    else{
        sem_post(&(cr->mutex));
        return -1;
    }
    
    free(cursor);

    //decrement num_client
    (cr->num_client)--;
    if ((cr->num_client) == 0){
        //num_client is empty
        sem_post(&(cr->num_client_mutex));
        cr->clients = NULL;
    }

    sem_post(&(cr->mutex));
    return 0;
}

/*
 * A thread calling this function will block in the call until
 * the number of registered clients has reached zero, at which
 * point the function will return.  Note that this function may be
 * called concurrently by an arbitrary number of threads.
 *
 * @param cr  The client registry.
 */
void creg_wait_for_empty(CLIENT_REGISTRY *cr){
    // waiting for the num_client become 0
   sem_wait(&(cr->num_client_mutex));

   //num_client is 0 after it
   debug("Empty Client Count");

   // unlock
   sem_post(&(cr->num_client_mutex));
}

/*
 * Shut down (using shutdown(2)) all the sockets for connections
 * to currently registered clients.  The file descriptors are not
 * unregistered by this function.  It is intended that the file
 * descriptors will be unregistered by the threads servicing their
 * connections, once those server threads have recognized the EOF
 * on the connection that has resulted from the socket shutdown.
 *
 * @param cr  The client registry.
 */
void creg_shutdown_all(CLIENT_REGISTRY *cr){
    sem_wait(&(cr->mutex));
    CLIENT* cursor = cr->clients;
    // interate and shutdowm
    while(cursor != NULL){
        shutdown(cursor->fd, SHUT_RDWR);  // Full shutdown of the socket
        cursor = cursor->next;
    }
    sem_post(&(cr->mutex));
}
