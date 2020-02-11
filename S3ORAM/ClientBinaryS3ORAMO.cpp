/*
 * ClientBinaryS3ORAMO.cpp
 *
 *  Created on: Mar 15, 2017
 *      Author: ceyhunozkaptan, thanghoang
 */

#include "ClientBinaryS3ORAMO.hpp"
#include "Utils.hpp"
#include "S3ORAM.hpp"


    
ClientBinaryS3ORAMO::ClientBinaryS3ORAMO() : ClientS3ORAM()
{
}

ClientBinaryS3ORAMO::~ClientBinaryS3ORAMO()
{
}

/**
 * Function Name: access
 *
 * Description: Starts access operation for a block with its ID to be retrived from distributed servers. 
 * This operations consists of several subroutines: generating shares for logical access vector, 
 * retrieving shares from servers, recovering secret block from shares, assigning new path for the block,
 * re-share/upload the block back to servers, run eviction subroutine acc. to EVICT_RATE
 * 
 * @param blockID: (input) ID of the block to be retrieved
 * @return 0 if successful
 */  
int ClientBinaryS3ORAMO::access(TYPE_ID blockID)
{
    S3ORAM ORAM;
	cout << "================================================================" << endl;
	cout << "STARTING ACCESS OPERATION FOR BLOCK-" << blockID <<endl; 
	cout << "================================================================" << endl;
	
    // 1. get the path & index of the block of interest
    TYPE_INDEX pathID = pos_map[blockID].pathID;
	cout << "	[ClientBinaryS3ORAMO] PathID = " << pathID <<endl;
    cout << "	[ClientBinaryS3ORAMO] Location = " << 	pos_map[blockID].pathIdx <<endl;
    
    // 2. create select query
	TYPE_DATA logicVector[(H+1)*BUCKET_SIZE];
	
	auto start = time_now;
    auto end = time_now;
	
	start = time_now;
	getLogicalVector(logicVector, blockID);
	end = time_now;
	cout<< "	[ClientBinaryS3ORAMO] Logical Vector Created in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<< " ns"<<endl;
    exp_logs[0] = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
	
	// 3. create query shares
	#if defined (PRECOMP_MODE) 
		start = time_now;
		for (TYPE_INDEX i = 0; i < (H+1)*BUCKET_SIZE; i++)
		{
			if (logicVector[i] == 0){
				for (int j = 0; j < NUM_SERVERS; j++){
					this->sharedVector[j][i] = this->precompZeros[j][0];
				}
			}
			else{
				for (int j = 0; j < NUM_SERVERS; j++){
					this->sharedVector[j][i] = this->precompOnes[j][0];
				}
			}
		}
		end = time_now;
		cout<< "	[ClientBinaryS3ORAMO] Shared Vector Created in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<< " ns"<<endl;
		exp_logs[1] = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
	#else //defined (PRECOMP_MODE) ================================================================================================
		start = time_now;
		ORAM.getSharedVector(logicVector, this->sharedVector,(H+1)*BUCKET_SIZE);
		end = time_now;
		cout<< "	[ClientBinaryS3ORAMO] Shared Vector Created in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<< " ns"<<endl;
		exp_logs[1] = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
	#endif //defined (PRECOMP_MODE) ================================================================================================
	
    
    
	// 4. send to server & receive the answer
    struct_socket thread_args[NUM_SERVERS];
    
    start = time_now;
    for (int i = 0; i < NUM_SERVERS; i++)
    {
        memcpy(&vector_buffer_out[i][0], &pathID, sizeof(pathID));
        memcpy(&vector_buffer_out[i][sizeof(pathID)], &this->sharedVector[i][0], (H+1)*BUCKET_SIZE*sizeof(TYPE_DATA));
        
		thread_args[i] = struct_socket(i, vector_buffer_out[i], sizeof(pathID)+(H+1)*BUCKET_SIZE*sizeof(TYPE_DATA), blocks_buffer_in[i], sizeof(TYPE_DATA)*DATA_CHUNKS,CMD_REQUEST_BLOCK,NULL);
            
		pthread_create(&thread_sockets[i], NULL, &ClientBinaryS3ORAMO::thread_socket_func, (void*)&thread_args[i]);
    }
    
    
    memset(recoveredBlock,0,sizeof(TYPE_DATA)*DATA_CHUNKS);
    for(int i = 0 ; i < NUM_SERVERS; i++)
    {
        memset(retrievedShare[i],0,sizeof(TYPE_DATA)*DATA_CHUNKS);
    }
    
    for (int i = 0; i < NUM_SERVERS; i++)
    {
        pthread_join(thread_sockets[i], NULL);
        memcpy(retrievedShare[i],blocks_buffer_in[i],sizeof(TYPE_DATA)*DATA_CHUNKS);
        cout << "	[ClientBinaryS3ORAMO] From Server-" << i+1 << " => BlockID = " << retrievedShare[i][0]<< endl;
    }
    end = time_now;
    cout<< "	[ClientBinaryS3ORAMO] All Shares Retrieved in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<< " ns"<<endl;
    exp_logs[2] = thread_max;
    thread_max = 0;
	
    // 5. recover the block
	start = time_now;
	ORAM.simpleRecover(retrievedShare, recoveredBlock);
	end = time_now;
	cout<< "	[ClientBinaryS3ORAMO] Recovery Done in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<< " ns"<<endl;
	exp_logs[3] = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
	
    
    cout << "	[ClientBinaryS3ORAMO] Block-" << recoveredBlock[0] <<" is Retrieved" <<endl;
    if (recoveredBlock[0] == blockID)
        cout << "	[ClientBinaryS3ORAMO] SUCCESS!!!!!!" << endl;
    else
        cout << "	[ClientBinaryS3ORAMO] ERROR!!!!!!!!" << endl;
		
    assert(recoveredBlock[0] == blockID && "ERROR: RECEIEVED BLOCK IS NOT CORRECT!!!!!!");
	
    
    // 6. update position map
    
    TYPE_INDEX fullPathIdx[H+1];
    ORAM.getFullPathIdx(fullPathIdx,pathID);
    this->metaData[fullPathIdx[pos_map[blockID].pathIdx / BUCKET_SIZE ]][pos_map[blockID].pathIdx % BUCKET_SIZE] = 0;
    
    // 6.1. assign to random path
    pos_map[blockID].pathID = Utils::RandBound(N_leaf)+(N_leaf-1);
    pos_map[blockID].pathIdx = numRead;
    this->metaData[0][numRead] = blockID;
    
    
    // 7. create new shares for the retrived block, 
    TYPE_DATA chunkShares[NUM_SERVERS];
    
    for(int u = 0 ; u < DATA_CHUNKS; u++ )
    {
        ORAM.createShares(recoveredBlock[u], chunkShares);
        
        for(int k = 0; k < NUM_SERVERS; k++) 
        {
                memcpy(&block_buffer_out[k][u*sizeof(TYPE_DATA)], &chunkShares[k], sizeof(TYPE_DATA));
        }
    }
    for ( int k = 0 ; k < NUM_SERVERS; k++)
    {
        memcpy(&block_buffer_out[k][DATA_CHUNKS*sizeof(TYPE_DATA)], &numRead, sizeof(TYPE_DATA));
    }
	// 8. upload the share to numRead-th slot in root bucket
    for(TYPE_INDEX k = 0; k < NUM_SERVERS; k++) 
    {
//		thread_args[k] = struct_socket(SERVER_ADDR[k]+ ":" + SERVER_PORT[k*NUM_SERVERS+k], block_buffer_out[k], sizeof(TYPE_DATA)*DATA_CHUNKS+sizeof(TYPE_INDEX), NULL, 0, CMD_SEND_BLOCK,NULL);
		thread_args[k] = struct_socket(k, block_buffer_out[k], sizeof(TYPE_DATA)*DATA_CHUNKS+sizeof(TYPE_INDEX), NULL, 0, CMD_SEND_BLOCK,NULL);

		pthread_create(&thread_sockets[k], NULL, &ClientBinaryS3ORAMO::thread_socket_func, (void*)&thread_args[k]);
    }
    
	this->numRead = (this->numRead+1)%EVICT_RATE;
    cout << "	[ClientBinaryS3ORAMO] Number of Read = " << this->numRead <<endl;
    
    for (int i = 0; i < NUM_SERVERS; i++)
    {
        pthread_join(thread_sockets[i], NULL);
        cout << "	[ClientBinaryS3ORAMO] Block upload completed!" << endl;
    }
    
	cout << "================================================================" << endl;
	cout << "ACCESS OPERATION FOR BLOCK-" << blockID << " COMPLETED." << endl; 
	cout << "================================================================" << endl;
	
    // 9. Perform eviction
    if(this->numRead == 0)
    {
        
        cout << "================================================================" << endl;
		cout << "STARTING EVICTION-" << this->numEvict+1 <<endl;
		cout << "================================================================" << endl;
        
        // 9.1. create permutation matrices
		for(TYPE_INDEX i = 0 ; i < H+1; i++)
        {
			memset(this->evictMatrix[i], 0, evictMatSize*sizeof(TYPE_DATA));
		}

		start = time_now;
        this->getEvictMatrix();
		end = time_now;
		cout<< "	[ClientBinaryS3ORAMO] Evict Matrix Created in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<< " ns"<<endl;
		exp_logs[5] = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
		
		
		// 9.2. create shares of  permutation matrices 
		cout<< "	[ClientBinaryS3ORAMO] Sharing Evict Matrix..." << endl;
		
		boost::progress_display show_progress((H+1)*evictMatSize);
        TYPE_DATA matrixShares[NUM_SERVERS];
        start = time_now;
        for (TYPE_INDEX i = 0; i < H+1; ++i) 
        {
            for (TYPE_INDEX j = 0; j < evictMatSize; ++j)
            {
                ORAM.createShares(this->evictMatrix[i][j], matrixShares);
                for (int k = 0; k < NUM_SERVERS; k++) 
                {   
                    this->sharedMatrix[k][i][j] = matrixShares[k];
                }
				++show_progress;
            }
        }
        end = time_now;
        cout<< "	[ClientBinaryS3ORAMO] Shared Matrix Created in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<< " ns"<<endl;
        exp_logs[6] = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
        
        // 9.3. send permutation matrices to servers
        start = time_now;
        for (int i = 0; i < NUM_SERVERS; i++)
        {
            for (TYPE_INDEX y = 0 ; y < H+1; y++)
            {
                memcpy(&evict_buffer_out[i][y*evictMatSize*sizeof(TYPE_DATA)], &this->sharedMatrix[i][y][0], evictMatSize*sizeof(TYPE_DATA));
            }
            memcpy(&evict_buffer_out[i][(H+1)*evictMatSize*sizeof(TYPE_DATA)], &numEvict, sizeof(TYPE_INDEX));
            
			thread_args[i] = struct_socket(i, evict_buffer_out[i], (H+1)*evictMatSize*sizeof(TYPE_DATA) + sizeof(TYPE_INDEX), NULL,0, CMD_SEND_EVICT,  NULL);
            pthread_create(&thread_sockets[i], NULL, &ClientBinaryS3ORAMO::thread_socket_func, (void*)&thread_args[i]);
        }
			
        for (int i = 0; i < NUM_SERVERS; i++)
        {
            pthread_join(thread_sockets[i], NULL);
        }
        end = time_now;
        cout<< "	[ClientBinaryS3ORAMO] Eviction DONE in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<< " ns"<<endl;
        
        exp_logs[8] = thread_max;
        thread_max = 0;
		
		cout << "================================================================" << endl;
		cout << "EVICTION-" << this->numEvict+1 << " COMPLETED" << endl;
		cout << "================================================================" << endl;
        
        this->numEvict = (numEvict+1) % N_leaf;
    }
    // 11. store local info to disk
	FILE* local_data = NULL;
	if((local_data = fopen(clientTempPath.c_str(),"wb+")) == NULL){
		cout<< "	[ClientBinaryS3ORAMO] File Cannot be Opened!!" <<endl;
		exit(0);
	}
	fwrite(this->pos_map, 1, (NUM_BLOCK+1)*sizeof(TYPE_POS_MAP), local_data);
	fwrite(&this->numEvict, sizeof(this->numEvict), 1, local_data);
	fwrite(&this->numRead, sizeof(this->numRead), 1, local_data);
	fclose(local_data);
     
	// 12. write log
	Utils::write_list_to_file(to_string(HEIGHT)+"_" + to_string(BLOCK_SIZE)+"_client_" + timestamp + ".txt",logDir, exp_logs, 9);
	memset(exp_logs, 0, sizeof(unsigned long int)*9);
	
	return 0;
}



/**
 * Function Name: getEvictMatrix
 *
 * Description: Generates logical eviction matrix to evict blocks from root to leaves according to 
 * eviction number and source, destination and sibling buckets by scanning position map.
 * 
 * @return 0 if successful
 */  
int ClientBinaryS3ORAMO::getEvictMatrix()
{
	S3ORAM ORAM;
    TYPE_INDEX* src_idx = new TYPE_INDEX[H];
    TYPE_INDEX* dest_idx = new TYPE_INDEX[H];
    TYPE_INDEX* sibl_idx = new TYPE_INDEX[H];
    
    string evict_str = ORAM.getEvictString(numEvict);
    
    ORAM.getEvictIdx(src_idx,dest_idx,sibl_idx,evict_str);
	cout<< "	[ClientBinaryS3ORAMO] Creating Evict Matrix..." << endl;
	boost::progress_display show_progress(H);
	
    for(int h = 1 ; h  < H+1; h++)
    {
        //for all real blocks in destination bucket, keep it as is
        TYPE_INDEX curDest_idx = dest_idx[h-1];
        TYPE_INDEX curSrc_idx = src_idx[h-1];
        TYPE_INDEX curSibl_idx = sibl_idx[h-1];
        vector<TYPE_ID> lstEmptyIdx;
        vector<TYPE_ID> lstEmptyIdxSibl; //only used when h =H 
        if( h == H) // this is for sibling bucket at leaf level
        {
            
            for ( int ii = 0 ; ii < BUCKET_SIZE ; ii++)
            {
                if(metaData[curSibl_idx][ii]!=0)
                {
                    TYPE_ID blockID = metaData[curSibl_idx][ii];
                    TYPE_INDEX j = pos_map[blockID].pathIdx;
                    evictMatrix[h][ (j-BUCKET_SIZE*h)*BUCKET_SIZE*2 + (j-BUCKET_SIZE*(h-1) )  ] = 1; 
                }
                else
                {
                    lstEmptyIdxSibl.push_back(ii);
                }   
            }
        }
        else 
        {
            for(int ii = 0 ; ii < BUCKET_SIZE; ii++)
            {
                metaData[curSibl_idx][ii]=0;
            }
        }
        for ( int ii = 0 ; ii < BUCKET_SIZE ; ii++)
        {
            if(metaData[curDest_idx][ii]!=0)
            {
                TYPE_ID blockID = metaData[curDest_idx][ii];
                TYPE_INDEX j = pos_map[blockID].pathIdx;
                evictMatrix[h-1][ (j-BUCKET_SIZE*h)*BUCKET_SIZE*2 + (j-BUCKET_SIZE*(h-1) )  ] = 1; 
            }
            else
            {
                lstEmptyIdx.push_back(ii);
            }   
        }
        for(int ii = 0 ; ii< BUCKET_SIZE ; ii++)
        {
            if(metaData[curSrc_idx][ii]!=0)
            {
                TYPE_ID blockID = metaData[curSrc_idx][ii];
                metaData[curSrc_idx][ii]=0;
                    
                TYPE_INDEX j = pos_map[blockID].pathIdx;
                TYPE_INDEX s = pos_map[blockID].pathID;
                TYPE_INDEX fullPath[H+1];
                ORAM.getFullPathIdx(fullPath,s);
                if( fullPath[h] == curDest_idx)
                {
                    if(lstEmptyIdx.size()==0)
                    {
                        cout<< "	[ClientBinaryS3ORAMO] Overflow!!!!. Please check the random generator or select smaller evict_rate"<<endl;
                        exit(0);
                    }
                    int emptyIdx = lstEmptyIdx[lstEmptyIdx.size()-1];
                    lstEmptyIdx.pop_back();
                    evictMatrix[h-1][ (emptyIdx)*BUCKET_SIZE*2 + (  j - BUCKET_SIZE*(h-1) ) ] = 1;     
                    pos_map[blockID].pathIdx = BUCKET_SIZE*h + emptyIdx;
                    metaData[curDest_idx][emptyIdx] = blockID;
                }
                else
                {
                    if(h<H)
                    {
                        pos_map[blockID].pathIdx= BUCKET_SIZE + j;
                        metaData[curSibl_idx][j%BUCKET_SIZE]=blockID;
                    }
                    else
                    {
                        if(lstEmptyIdxSibl.size()==0)
                        {
                            cout<< "	[ClientBinaryS3ORAMO] Overflow!!!, Please check the random generator or select smaller evict_rate"<<endl;
                            exit(0);
                        }
                        int emptyIdx = lstEmptyIdxSibl[lstEmptyIdxSibl.size()-1];
                        lstEmptyIdxSibl.pop_back();
                        evictMatrix[H][ (emptyIdx)*BUCKET_SIZE*2 + (  j - BUCKET_SIZE*(h-1) ) ] = 1;     
                        pos_map[blockID].pathIdx = BUCKET_SIZE*h + emptyIdx;
                        metaData[curSibl_idx][emptyIdx] = blockID;
                        
                    }
                }
            }
        }
		++show_progress;
    }
    
	return 0;
}


