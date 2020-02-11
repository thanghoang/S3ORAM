/*
 * ServerBinaryS3ORAMO.hpp
 *
 *  Created on: Apr 7, 2017
 *      Author: ceyhunozkaptan, thanghoang
 */

#ifndef SERVERBINARYS3ORAMO_HPP
#define SERVERBINARYS3ORAMO_HPP

#include "ServerS3ORAM.hpp"


#include "config.h"
#include <zmq.hpp>
#include <pthread.h>



class ServerBinaryS3ORAMO : public ServerS3ORAM
{
private:
    static void* thread_loadTripletData_func(void* args);
    

    
public:
    ServerBinaryS3ORAMO(); 
    ServerBinaryS3ORAMO(TYPE_INDEX serverNo, int selectedThreads); 
    ~ServerBinaryS3ORAMO();

    
    // main functions
    int retrieve(zmq::socket_t& socket);
    int evict(zmq::socket_t& socket);

    // eviction subroutine
    int multEvictTriplet( zz_p** evictMatrix);
    
    
};

#endif // SERVERS3ORAM_HPP
