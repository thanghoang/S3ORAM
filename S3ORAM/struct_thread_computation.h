/*
 * struct_thread_crossProd.h
 *
 *  Created on: Apr 27, 2017
 *      Author: ceyhunozkaptan, thanghoang
 */
 
#ifndef STRUCT_THREAD_CROSSPRODUCT_H
#define  STRUCT_THREAD_CROSSPRODUCT_H

#include "config.h"
typedef struct struct_thread_computation
{
    //thread
    TYPE_INDEX startIdx,endIdx;
	
    //retrieval
	zz_p** data_vector;
    zz_p* select_vector;    // dot product select vector
    TYPE_DATA* dot_product_output;        //dot product output
    
    // eviction
	zz_p** data_vector_triplet;
    zz_p** evict_matrix;    // cross product matrix
    TYPE_DATA** cross_product_output;  // cross product output
    
    struct_thread_computation(TYPE_INDEX start, TYPE_INDEX end, zz_p** input, zz_p* select_vector, TYPE_DATA* dotOutput)
	{
		this->select_vector = select_vector;
        this->data_vector = input;
        startIdx = start;
        endIdx = end;
        this->dot_product_output = dotOutput;
	}
    
    
    struct_thread_computation(TYPE_INDEX startMat, TYPE_INDEX endMat, zz_p** input, zz_p** evicMat, TYPE_DATA** crossOutput)
	{
		this->evict_matrix = evicMat;
        this->data_vector_triplet = input;
        startIdx = startMat;
        endIdx = endMat;
        this->cross_product_output = crossOutput;
	}
	
	struct_thread_computation()
	{
	}
	~struct_thread_computation()
	{
	}

}THREAD_COMPUTATION;

#endif