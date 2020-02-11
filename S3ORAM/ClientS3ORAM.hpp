#ifndef CLIENTS3ORAM_HPP
#define CLIENTS3ORAM_HPP



#include "config.h"
#include <pthread.h>
#include "zmq.hpp"
#include "struct_socket.h"


class ClientS3ORAM
{

protected:

    //client storage for ORAM operations
    TYPE_ID** metaData; //this is for position_map scanning optimization
    TYPE_POS_MAP* pos_map;

#if defined(CORAM_LAYOUT)    
    TYPE_DATA** STASH;
    TYPE_INDEX* metaStash;
    
#endif
    //variables for retrieval operation
	TYPE_INDEX numRead;
	TYPE_DATA** sharedVector;
    
    TYPE_DATA** retrievedShare;
    TYPE_DATA* recoveredBlock;
    
    unsigned char** vector_buffer_out;	
    unsigned char** block_buffer_out; 
    unsigned char** blocks_buffer_in;
    
    //variables for eviction
    TYPE_INDEX numEvict;
    TYPE_DATA*** sharedMatrix;
	TYPE_DATA** evictMatrix; 
    unsigned char** evict_buffer_out;

    //thread
	pthread_t thread_sockets[NUM_SERVERS];

#if defined(PRECOMP_MODE)
	TYPE_DATA** precompOnes;
	TYPE_DATA** precompZeros;
#endif


public:
    ClientS3ORAM();
    ~ClientS3ORAM();
    
    
    
    static zmq::context_t **context;
    static zmq::socket_t **socket;
    
       
    
    //main functions
    
    virtual int access(TYPE_ID blockID) = 0;
    
    virtual int init();
    
    virtual int loadState();
    virtual int saveState();
    int sendORAMTree();
    
    //retrieval_vector
    virtual int getLogicalVector(TYPE_DATA* logicalVector, TYPE_ID blockID);
    
    //eviction_matrix
    virtual int getEvictMatrix() =0;
    
    //socket
	static void* thread_socket_func(void* args);	
    static int sendNrecv(int peer_idx, unsigned char* data_out, size_t data_out_size, unsigned char* data_in, size_t data_in_size, int CMD);

    //logging
	static unsigned long int exp_logs[9]; 	
    static unsigned long int thread_max;
	static char timestamp[16];    
};

#endif // CLIENTS3ORAM_HPP
