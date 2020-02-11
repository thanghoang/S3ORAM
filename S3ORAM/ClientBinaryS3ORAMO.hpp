/*
 * ClientBinaryS3ORAMO.hpp
 *
 *  Created on: Mar 15, 2017
 *      Author: thanghoang
 */
 
#ifndef CLIENTBINARYS3ORAMO_HPP
#define CLIENTBINARYS3ORAMO_HPP

#include "ClientS3ORAM.hpp"
#include "config.h"


class ClientBinaryS3ORAMO : public ClientS3ORAM
{
private:

public:
    ClientBinaryS3ORAMO();
    ~ClientBinaryS3ORAMO();
    
    
    int access(TYPE_ID blockID);
    //eviction_matrix
    int getEvictMatrix();

};

#endif // CLIENTORAM_HPP
