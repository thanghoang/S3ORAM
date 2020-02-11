/*
 * ServerKaryS3ORAMO.hpp
 *
 *  Created on: Apr 7, 2017
 *      Author: ceyhunozkaptan, thanghoang
 */

#ifndef SERVERKARYS3ORAMO_HPP
#define SERVERKARYS3ORAMO_HPP

#include "ServerBinaryS3ORAMO.hpp"

class ServerKaryS3ORAMO : public ServerBinaryS3ORAMO
{
private: 
   
public:

    //specific
    int evict(zmq::socket_t& socket);

    int retrieve(zmq::socket_t& socket);
    
     // eviction subroutine
    int multEvictTriplet(int level);

    
    ServerKaryS3ORAMO() ; 
    ServerKaryS3ORAMO(TYPE_INDEX serverNo, int selectedThreads); 
    ~ServerKaryS3ORAMO();   

};

#endif // SERVERS3ORAM_HPP
