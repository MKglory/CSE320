#include "server.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <string.h>
#include <unistd.h>
#include "client_registry.h"
#include "data.h"
#include "debug.h"
#include "protocol.h"
#include "store.h"
#include "transaction.h"

/*
 * Client registry that should be used to track the set of
 * file descriptors of currently connected clients.
 */
int PUT_REQUEST(int fd, TRANSACTION** trans_ptr, uint32_t serial);
int GET_REQUEST(int fd, TRANSACTION** trans_ptr, uint32_t serial);
void COMMIT_REQUEST(int fd, TRANSACTION** trans_ptr, uint32_t serial);
int check_abort(TRANSACTION* trans);
void reply(BLOB* blob, int fd, TRANSACTION** trans_ptr, uint32_t serial);

// extern CLIENT_REGISTRY *client_registry;
CLIENT_REGISTRY *client_registry;

/*
 * Thread function for the thread that handles client requests.
 *
 * @param  Pointer to a variable that holds the file descriptor for
 * the client connection.  This pointer must be freed once the file
 * descriptor has been retrieved.
 */
void *xacto_client_service(void *arg){
    int fd;
    TRANSACTION* trans;
    uint32_t serial;
    // rereived the file descriptor
    fd = *((int*)arg);
    // free the pointer
    free(arg);

    // thread resources are automatically released when the thread exits
    pthread_detach(pthread_self());

    // register the client file descriptor
    creg_register(client_registry, fd);
    //transaction
    trans = trans_create();
    XACTO_PACKET* pkt = malloc(sizeof(XACTO_PACKET));
    void* first_data;
    int stop = 0; // check whether the client commit, used to disconnected
    int temp = 0; // temp variable for stop
    // keep connected with client
    while(stop == 0){
        temp = check_abort(trans);
        if (temp == 1)
            stop = temp;
        temp = 0;
        //retreieve packet
        if (proto_recv_packet(fd, pkt, &first_data) == 0){
            // Store serial number
            serial = pkt->serial;
            // once receive first packet, check its packet type
            if (pkt->type == XACTO_PUT_PKT){
                temp = check_abort(trans);
                if (temp == 1){
                    stop = temp;
                    temp = 0;
                    break;
                }
                // if it is put request
                debug("PUT REQUEST");
                stop = PUT_REQUEST(fd, &trans, serial);
                store_show();
                temp = check_abort(trans);
                if (temp == 1){
                    stop = temp;
                    temp = 0;
                    break;
                }
            }
            else if (pkt->type == XACTO_GET_PKT){
                temp = check_abort(trans);
                if (temp == 1){
                    stop = temp;
                    temp = 0;
                    break;
                }
                // if it is get request
                debug("GET REQUEST");
                stop = GET_REQUEST(fd, &trans, serial);
                temp = check_abort(trans);
                if (temp == 1){
                    stop = temp;
                    temp = 0;
                    break;
                }
            }
            else if (pkt->type == XACTO_COMMIT_PKT){
                // this is commit request
                temp = check_abort(trans);
                if (temp == 1){
                    stop = temp;
                    temp = 0;
                    break;
                }
                debug("COMMIT REQUEST");
                COMMIT_REQUEST(fd, &trans, serial);
                stop = 1;
                temp = check_abort(trans);
                if (temp == 1){
                    stop = temp;
                    temp = 0;
                    break;
                }
            }
        }
        else{
            // fail to retrieve fisrst data packet
            trans_abort(trans);
            stop = 1;
            //XACTO_PACKET* pkt = malloc(sizeof(XACTO_PACKET));
            free(pkt);
            creg_unregister(client_registry, fd);
        }
    }
    if (pkt != NULL){
        free(pkt);
    }
    return NULL;
}

// return 0 mean sucessful. return 1 means fail
int PUT_REQUEST(int fd, TRANSACTION** trans_ptr, uint32_t serial){
    TRANSACTION* trans = *trans_ptr;
    //retrieve key packet
    XACTO_PACKET* key_pkt = malloc(sizeof(XACTO_PACKET));
    XACTO_PACKET* value_pkt = malloc(sizeof(XACTO_PACKET));
    void* second_pkt = NULL; // key packet
    void* third_pkt = NULL; // value packet
    if(proto_recv_packet(fd, key_pkt, &second_pkt) == 0){
        // if (key_pkt->type != XACTO_KEY_PKT){
        //     debug("packet type is not matched.");
        //     trans_abort(trans);
        //     return 1;
        // }
        debug("Second packet is %s", (char*) second_pkt);
        // create blob
        BLOB* key_blob = blob_create((char*)second_pkt, ntohl(key_pkt->size));
        // create key
        KEY* key = key_create(key_blob);

        // receiving third packet （value packet)
        if (proto_recv_packet(fd, value_pkt, &third_pkt) ==0){
            // if (key_pkt->type != XACTO_VALUE_PKT){
            //     debug("packet type is not matched.");
            //     trans_abort(trans);
            //     return 1;
            // }
            debug("Third packet is %s", (char*) third_pkt);
            //create blob
            BLOB* data_blob = blob_create((char*)third_pkt, ntohl(value_pkt->size));
            //put it in the store
            store_put(trans, key, data_blob);

            //once received packet all set, reply packet
            reply(NULL, fd, trans_ptr, serial);
            free(key_pkt);
            free(value_pkt);

        }
        else{
            //fail to received value packet
            trans_abort(trans);
            return 1;
        }
    }
    else{
        trans_abort(trans);
        // fail to received key packet
        return 1;
    }
    return 0;
}

int GET_REQUEST(int fd, TRANSACTION** trans_ptr, uint32_t serial){
    TRANSACTION* trans = *trans_ptr;
    // Get value pacaket
    XACTO_PACKET* value_pkt = malloc(sizeof(XACTO_PACKET));
    void* data = NULL;
    if (proto_recv_packet(fd, value_pkt, &data) == 0){
         // if (key_pkt->type != XACTO_VALUE_PKT){
            //     debug("packet type is not matched.");
            //     trans_abort(trans);
            //     return 1;
        // }
        //create blob
        store_show();
        BLOB* blob = blob_create((char*)data, ntohl(value_pkt->size));
        //create key
        KEY* key = key_create(blob);
        
        // BLOB* value = malloc(sizeof(BLOB));
        BLOB* value;
        // get value from store
        store_get(trans, key, &value);

        //send reply 
        reply(NULL, fd, trans_ptr, serial);
      
        //send value;
        if (value != NULL){
                // //create key
                // char* data1 = data;;
                // BLOB* keyBlob = blob_create(data1, ntohl(value_pkt->size));
                // key = key_create(keyBlob);
                // blob = blob_create((char*)value->content, value->size);
                // store_put(trans, key, value);
                debug("reply data is %s",value->content);
                reply(value, fd, trans_ptr, serial);
                store_show();
        }
        else{
            XACTO_PACKET* reply = malloc(sizeof(XACTO_PACKET));  
            //get request and reply with NULL dara
            reply->serial = serial;
            reply->type = XACTO_VALUE_PKT;
            reply->status = 0;
            reply->null = 1;
            reply->size = 0;
            reply->size = htonl(reply->size);
            if(proto_send_packet(fd, reply, value) != 0){
                debug("Replying Failed.");
                trans_abort(trans);
            }
            free(reply);
        }

        // key_dispose(key);
        free(value_pkt);
        // free(value);
        
    }
    else{
        //fail to received value packet
        trans_abort(trans);
        return 1;
    }
    return 0;
}

void COMMIT_REQUEST(int fd, TRANSACTION** trans_ptr, uint32_t serial){
    TRANSACTION* trans = *trans_ptr;
    if (trans_commit(trans) == TRANS_COMMITTED){
        // commit succesful
        // reply packet
        reply(NULL, fd, trans_ptr, serial);
    }

}

void reply(BLOB* blob, int fd, TRANSACTION** trans_ptr, uint32_t serial){
    debug("Repling packet to client");
    TRANSACTION* trans = *trans_ptr;
    XACTO_PACKET* reply = malloc(sizeof(XACTO_PACKET));  
    void* data = NULL;
    reply->serial = serial;
    reply->type = XACTO_REPLY_PKT;
    reply->status = trans_get_status(trans);
    debug("Status is %u\n", reply->status);
    if (blob == NULL){
        reply->null = 0;
        reply->size = 0;
    }
    else{
        // Set reply packet
        data = blob->content;
        if (data == NULL){
            reply->null = 1;
            reply->size = 0;
        }
        else{ 
            reply->null = 0;
            reply->size = blob->size;
            reply->type = XACTO_VALUE_PKT;
        }
    }
    debug("Reply data is %s", (char*) data);
    debug("Reply data size is %u\n", reply->size);
    debug("reply data serial is  %u\n", reply->serial);
    reply->size = htonl(reply->size);
    if(proto_send_packet(fd, reply, data) != 0){
        debug("Replying Failed.");
        trans_abort(trans);
    }
    // free(data);
    free(reply);
}
int check_abort(TRANSACTION* trans){
    int stop = 0;
    if (trans_get_status(trans) == TRANS_ABORTED){
        stop = 1;
    }
    return stop;
}

