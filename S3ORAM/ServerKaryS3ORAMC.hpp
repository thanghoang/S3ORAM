#ifndef SERVERKARYS3CORAMC_H
#define SERVERKARYS3CORAMC_H

#include "ServerS3ORAM.hpp"

#include "config.h"
#include <zmq.hpp>
#include <pthread.h>

class ServerKaryS3ORAMC : public ServerS3ORAM
{
private:
        //variables for eviction
    TYPE_DATA**** ownShares_coram;     
    
    TYPE_DATA*** BUCKET_DATA_coram;
    zz_p**** evictMatrix_coram;
    zz_p*** cross_product_vector_coram;
    pthread_t** thread_compute_coram;
    
        
public:
    
    //specific
    int evict(zmq::socket_t& socket);
    static void* thread_readBucket_func(void* args);
    
    
   
    //inherent
    ServerKaryS3ORAMC(); 
    ServerKaryS3ORAMC(TYPE_INDEX serverNo, int selectedThreads); 
    ~ServerKaryS3ORAMC();

    // main functions
    int retrieve(zmq::socket_t& socket); //MERGE with Onion-ORAM


    // eviction subroutine
    int multEvictTriplet(int level, int es, int ee);    
};

#endif // SERVERS3CORAM_H
