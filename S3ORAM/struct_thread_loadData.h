/*
 * struct_thread_loadData.h
 *
 *  Created on: May 1, 2017
 *      Author: ceyhunozkaptan, thanghoang
 */
 
#ifndef STRUCT_THREAD_LOADDATA_H
#define  STRUCT_THREAD_LOADDATA_H

#include "config.h"
typedef struct struct_thread_loadData
{
    int serverNo;
	zz_p** data_vector;
   
    //thread
    TYPE_INDEX startIdx,endIdx;
    
    //retrieval
    TYPE_INDEX* fullPathIdx; 
    int fullPathIdx_length;

    //eviction
    TYPE_INDEX idx; 
    
    
    //for triplet
    TYPE_INDEX destIdx;
    //for retrieval
     struct_thread_loadData(int serverNo, TYPE_INDEX start, TYPE_INDEX end, zz_p** data_vector, TYPE_INDEX* fullPathIdx, int fullPathIdx_length)
    {
        this->serverNo = serverNo;
        this->startIdx = start;
        this->endIdx = end;
        this->data_vector = data_vector;
        this->fullPathIdx = fullPathIdx;
        this->fullPathIdx_length = fullPathIdx_length;
    }
    
    //for kary eviction
    struct_thread_loadData(int serverNo, TYPE_INDEX start, TYPE_INDEX end, zz_p** data_vector, TYPE_INDEX bucketIdx)
	{
		this->serverNo = serverNo;
        this->startIdx = start;
        this->endIdx = end;
        this->data_vector = data_vector;    
        this->idx = bucketIdx;
    }
    
    //for triplet eviction
    struct_thread_loadData(int serverNo, TYPE_INDEX start, TYPE_INDEX end, zz_p** data_vector, TYPE_INDEX srcIdx, TYPE_INDEX destIdx)
	{
		this->serverNo = serverNo;
        this->startIdx = start;
        this->endIdx = end;
        this->data_vector = data_vector;    
        this->idx = srcIdx;
        this->destIdx = destIdx;
    }
    
	struct_thread_loadData()
	{
	}
	~struct_thread_loadData()
	{
	}

}THREAD_LOADDATA;
#endif