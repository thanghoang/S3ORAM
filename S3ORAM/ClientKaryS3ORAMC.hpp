#ifndef CLIENTKARYS3CORAMC_H
#define CLIENTKARYS3CORAMC_H

#include "ClientS3ORAM.hpp"

class ClientKaryS3ORAMC : public ClientS3ORAM
{
private:

    //specific variable for S3CORAM
    TYPE_DATA** STASH;
    TYPE_INDEX* metaStash;
    
    
    int deepest[HEIGHT+2];
	int target[HEIGHT+2]; 
	int deepestIdx[HEIGHT+2];
    
    int countNumBlockInStash();

public:

    ClientKaryS3ORAMC();
    ~ClientKaryS3ORAMC();
    
    
    
    //main functions
    int loadState();
    int saveState();
    int access(TYPE_ID blockID);
    
    int init();
    //retrieval_vector
    int getLogicalVector(TYPE_DATA* logicalVector, TYPE_ID blockID);

    //eviction_matrix
    int getEvictMatrix();
    

};

#endif // CLIENTS3CORAM_H
