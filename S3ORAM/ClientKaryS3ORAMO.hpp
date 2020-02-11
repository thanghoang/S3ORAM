/*
 * ClientKaryS3ORAMO.hpp
 *
 *  Created on: Mar 15, 2017
 *      Author: ceyhunozkaptan, thanghoang
 */
 
#ifndef CLIENTKARYS3ORAMO_HPP
#define CLIENTKARYS3ORAMO_HPP

#include "ClientS3ORAM.hpp"





class ClientKaryS3ORAMO : public ClientS3ORAM
{
private:

public:
    ClientKaryS3ORAMO();
    ~ClientKaryS3ORAMO();
    
    
    
    //specific
    int access(TYPE_ID blockID);
    int init(); 
    //retrieval_vector
    int getLogicalVector(TYPE_DATA* logicalVector, TYPE_ID blockID);
    //eviction_matrix
    int getEvictMatrix();   
    
};

#endif // CLIENTORAM_HPP
