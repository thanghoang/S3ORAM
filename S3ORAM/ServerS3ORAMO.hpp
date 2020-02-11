/*
 * ServerS3ORAM.hpp
 *
 *  Created on: Apr 7, 2017
 *      Author: ceyhunozkaptan, thanghoang
 */

#ifndef SERVERS3ORAM_HPP
#define SERVERS3ORAM_HPP

#include "config.h"
#include <zmq.hpp>
#include <pthread.h>

class ServerS3ORAM
{
private:
    //specific
    unsigned char* block_buffer_in;
    
    
    //inherent
    //local variable
    std::string CLIENT_ADDR;
    TYPE_INDEX serverNo;
    TYPE_INDEX others[NUM_SERVERS-1];
    TYPE_DATA*** ownShares;     

    //variables for retrieval
    zz_p** dot_product_vector;
    unsigned char* select_buffer_in;
    TYPE_DATA* sumBlock;
    
    //variables for eviction
    TYPE_DATA** BUCKET_DATA;
    zz_p*** evictMatrix;
    //zz_p*** evictMatrix[H+1][BUCKET_SIZE][2*BUCKET_SIZE];
    zz_p** cross_product_vector;
    zz_p** cross_product_vector_aux;

    //thread
    int numThreads;
    pthread_t* thread_compute;
    pthread_t thread_recv[NUM_SERVERS-1];
    pthread_t thread_send[NUM_SERVERS-1];
    
    //socket 
    unsigned char* evict_buffer_in;    
    unsigned char* block_buffer_out;
    unsigned char* bucket_buffer; 
    
    unsigned char** shares_buffer_in;
    unsigned char** shares_buffer_out;

public:

    //specific
    int evict(zmq::socket_t& socket);
    int recvBlock(zmq::socket_t& socket); 
    
    
    static zmq::context_t **context_send;
    static zmq::socket_t **socket_send;
    
    static zmq::context_t **context_recv;
    static zmq::socket_t **socket_recv;
    
    
    
    
    
    
    //inherent
    ServerS3ORAM(); 
    ServerS3ORAM(TYPE_INDEX serverNo, int selectedThreads); 
    ~ServerS3ORAM();

    int start();
    
    // main functions
    int retrieve(zmq::socket_t& socket);
    int recvORAMTree(zmq::socket_t& socket);

    // retrieval subroutine 
    static void* thread_dotProduct_func(void* args);

    // eviction subroutine
    int multEvictTriplet(int level);

    static int send(int peer_idx, unsigned char* input, size_t inputSize);
    static int recv(int peer_idx, unsigned char* output, size_t outputSize);

    //thread functions
    static void* thread_crossProduct_func(void* args);
    static void* thread_socket_func(void* args);
    static void* thread_loadRetrievalData_func(void* args);
    static void* thread_loadBucket_func(void* args);
    
    static unsigned long int server_logs[13]; 
    static unsigned long int thread_max;
    static char timestamp[16];
};

#endif // SERVERS3ORAM_HPP
