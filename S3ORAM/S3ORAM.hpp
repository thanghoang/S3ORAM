/*
 * SSORAM.hpp
 *
 *  Created on: Mar 15, 2017
 *      Author: ceyhunozkaptan, thanghoang
 */
 
#ifndef SSORAM_HPP
#define SSORAM_HPP

#include "config.h"

class S3ORAM
{
public:
    S3ORAM();
    ~S3ORAM();
    
    int build(TYPE_POS_MAP* pos_map, TYPE_ID** metaData);

    int getEvictIdx (TYPE_INDEX *srcIdx, TYPE_INDEX *destIdx, TYPE_INDEX *siblIdx, string evict_str);
    
	string getEvictString(TYPE_ID n_evict);

	int getFullPathIdx(TYPE_INDEX* fullPath, TYPE_INDEX pathID);

    int createShares(TYPE_DATA input, TYPE_DATA* output);

	int getSharedVector(TYPE_DATA* logicVector, TYPE_DATA** sharedVector, int vector_len);

	int simpleRecover(TYPE_DATA** shares, TYPE_DATA* result);

	int precomputeShares(TYPE_DATA input, TYPE_DATA** output, TYPE_INDEX output_size);
    
    
    //Circuit-ORAM layout
    int getFullEvictPathIdx(TYPE_INDEX *fullPathIdx, string str_evict);
    int getDeepestLevel(TYPE_INDEX PathID, TYPE_INDEX blockPathID);
    void getDeepestBucketIdx(TYPE_INDEX* meta_stash, TYPE_INDEX* meta_path, TYPE_INDEX evictPathID, int* output);
    int prepareDeepest(TYPE_INDEX* meta_stash, TYPE_INDEX* meta_path, TYPE_INDEX PathID, int* deepest);
    int getEmptySlot(TYPE_INDEX* meta_path, int level);
    int prepareTarget(TYPE_INDEX* meta_path, TYPE_INDEX pathID, int *deepest, int* target);
};
    
#endif // SSORAM_HPP
