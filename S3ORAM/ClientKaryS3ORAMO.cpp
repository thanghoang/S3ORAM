/*
 * ClientKaryS3ORAMO.cpp
 *
 *  Created on: Mar 15, 2017
 *      Author: ceyhunozkaptan, thanghoang
 */

#include "ClientKaryS3ORAMO.hpp"
#include "Utils.hpp"
#include "S3ORAM.hpp"



ClientKaryS3ORAMO::ClientKaryS3ORAMO() : ClientS3ORAM()
{
    
    for (TYPE_INDEX i = 0 ; i < NUM_SERVERS; i++)
	{
        delete this->sharedMatrix[i][H];
        this->sharedMatrix[i][H] = new TYPE_DATA[2*evictMatSize];
	}
	delete this->evictMatrix[H];
    this->evictMatrix[H] = new TYPE_DATA[2*evictMatSize];
    
    //H+2, instead of H+1;
    for (TYPE_INDEX i = 0 ; i < NUM_SERVERS ; i++)
    {
        delete this->evict_buffer_out[i];
        this->evict_buffer_out[i] = new unsigned char[(H+2)*evictMatSize*sizeof(TYPE_DATA) + sizeof(TYPE_INDEX)];
    }
    

	for(int i = 0; i < NUM_SERVERS; i++)
	{
        this->sharedVector[i];
		this->sharedVector[i] = new TYPE_DATA[(H+2)*BUCKET_SIZE];
	}
    for (TYPE_INDEX i = 0 ; i < NUM_SERVERS ; i++)
    {
        this->vector_buffer_out[i];
        this->vector_buffer_out[i] = new unsigned char[sizeof(TYPE_INDEX)+(H+2)*BUCKET_SIZE*sizeof(TYPE_DATA)]; 
    }
}

ClientKaryS3ORAMO::~ClientKaryS3ORAMO()
{
}


/**
 * Function Name: init
 *
 * Description: Initialize shared ORAM data on disk storage of the client
 * and creates logging and configuration files
 * 
 * @return 0 if successful
 */ 
int ClientKaryS3ORAMO::init()
{
    ClientS3ORAM::init();
    for(unsigned long long i = 1 ; i < NUM_BLOCK+1; i++)
    {
        this->pos_map[i].pathIdx += BUCKET_SIZE;
        this->pos_map[i].pathID -= N_leaf;
    }
	saveState();
	
    return 0;
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
int ClientKaryS3ORAMO::access(TYPE_ID blockID)
{
    S3ORAM ORAM;
    cout << "================================================================" << endl;
	cout << "STARTING ACCESS OPERATION FOR BLOCK-" << blockID <<endl; 
	cout << "================================================================" << endl;
	
    // 1. get the path & index of the block of interest
    TYPE_INDEX pathID = pos_map[blockID].pathID;
	cout << "	[ClientKaryS3ORAMO] PathID = " << pathID <<endl;
    cout << "	[ClientKaryS3ORAMO] Location = " << 	pos_map[blockID].pathIdx <<endl;
    // 2. create select query
	TYPE_DATA logicVector[(H+2)*BUCKET_SIZE];
	
	auto start = time_now;
    auto end = time_now;
	
	start = time_now;
	getLogicalVector(logicVector, blockID);
	end = time_now;
	cout<< "	[ClientKaryS3ORAMO] Logical Vector Created in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<< " ns"<<endl;
    exp_logs[0] = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
	
	// 3. create query shares
	#if defined (PRECOMP_MODE) 
		start = time_now;
		for (TYPE_INDEX i = 0; i < (H+2)*BUCKET_SIZE; i++)
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
		cout<< "	[ClientKaryS3ORAMO] Shared Vector Created in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<< " ns"<<endl;
		exp_logs[1] = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
	#else //defined (PRECOMP_MODE) ================================================================================================
		start = time_now;
		ORAM.getSharedVector(logicVector, this->sharedVector, (H+2)*BUCKET_SIZE);
		end = time_now;
		cout<< "	[ClientKaryS3ORAMO] Shared Vector Created in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<< " ns"<<endl;
		exp_logs[1] = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
	#endif //defined (PRECOMP_MODE) ================================================================================================
	
    
    
	// 4. send to server & receive the answer
    struct_socket thread_args[NUM_SERVERS];
    
    start = time_now;
    for (int i = 0; i < NUM_SERVERS; i++)
    {
        memcpy(&vector_buffer_out[i][0], &pathID, sizeof(pathID));
        memcpy(&vector_buffer_out[i][sizeof(pathID)], &this->sharedVector[i][0], (H+2)*BUCKET_SIZE*sizeof(TYPE_DATA));
     	thread_args[i] = struct_socket(i, vector_buffer_out[i], sizeof(pathID)+(H+2)*BUCKET_SIZE*sizeof(TYPE_DATA), blocks_buffer_in[i], sizeof(TYPE_DATA)*DATA_CHUNKS,CMD_REQUEST_BLOCK,NULL);

		pthread_create(&thread_sockets[i], NULL, &ClientKaryS3ORAMO::thread_socket_func, (void*)&thread_args[i]);
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
        cout << "	[ClientKaryS3ORAMO] From Server-" << i+1 << " => BlockID = " << retrievedShare[i][0]<< endl;
    }
    end = time_now;
    cout<< "	[ClientKaryS3ORAMO] All Shares Retrieved in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<< " ns"<<endl;
    exp_logs[2] = thread_max;
    thread_max = 0;
	
    // 5. recover the block
	start = time_now;
	ORAM.simpleRecover(retrievedShare, recoveredBlock);
	end = time_now;
	cout<< "	[ClientKaryS3ORAMO] Recovery Done in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<< " ns"<<endl;
	exp_logs[3] = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
	
    
    cout << "	[ClientKaryS3ORAMO] Block-" << recoveredBlock[0] <<" is Retrieved" <<endl;
    if (recoveredBlock[0] == blockID)
        cout << "	[ClientKaryS3ORAMO] SUCCESS!!!!!!" << endl;
    else
        cout << "	[ClientKaryS3ORAMO] ERROR!!!!!!!!" << endl;
		
    assert(recoveredBlock[0] == blockID && "ERROR: RECEIEVED BLOCK IS NOT CORRECT!!!!!!");
	
    // 6. update position map
    
    TYPE_INDEX fullPathIdx[H+2];
    ORAM.getFullPathIdx(fullPathIdx,pathID);
    
    fullPathIdx[H+1] = fullPathIdx[H] + pow(K_ARY,HEIGHT);
    
    this->metaData[fullPathIdx[pos_map[blockID].pathIdx / BUCKET_SIZE ]][pos_map[blockID].pathIdx % BUCKET_SIZE] = 0;
    
    // 6.1. assign to random path
    pos_map[blockID].pathID = Utils::RandBound(N_leaf) + (NUM_NODES-2*N_leaf);
    
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
		thread_args[k] = struct_socket(k, block_buffer_out[k], sizeof(TYPE_DATA)*DATA_CHUNKS+sizeof(TYPE_INDEX), NULL, 0, CMD_SEND_BLOCK,NULL);
        pthread_create(&thread_sockets[k], NULL, &ClientKaryS3ORAMO::thread_socket_func, (void*)&thread_args[k]);
    }
    for (int i = 0; i < NUM_SERVERS; i++)
    {
        pthread_join(thread_sockets[i], NULL);
        cout << "	[ClientKaryS3ORAMO] Block upload completed!" << endl;
    }
    
    
	cout << "================================================================" << endl;
	cout << "ACCESS OPERATION FOR BLOCK-" << blockID << " COMPLETED." << endl; 
	cout << "================================================================" << endl;
	
    this->numRead = (this->numRead+1)%EVICT_RATE;
    cout << "	[ClientKaryS3ORAMO] Number of Read = " << this->numRead <<endl;
    
    // 9. Perform eviction
    if(this->numRead == 0)
    {
        cout << "================================================================" << endl;
		cout << "STARTING EVICTION-" << this->numEvict+1 <<endl;
		cout << "================================================================" << endl;
        
        // 9.1. create permutation matrices
		for(TYPE_INDEX i = 0 ; i < H; i++)
        {
			memset(this->evictMatrix[i], 0, evictMatSize*sizeof(TYPE_DATA));
		}
        memset(this->evictMatrix[H], 0, 2*evictMatSize*sizeof(TYPE_DATA));
		
		start = time_now;
        this->getEvictMatrix();
		end = time_now;
		
        cout<< "	[ClientKaryS3ORAMO] Evict Matrix Created in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<< " ns"<<endl;
		exp_logs[5] = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
		
		// 9.2. create shares of  permutation matrices 
		cout<< "	[ClientKaryS3ORAMO] Sharing Evict Matrix..." << endl;
		
		boost::progress_display show_progress((H+1)*evictMatSize);
        TYPE_DATA matrixShares[NUM_SERVERS];
        start = time_now;
        for (TYPE_INDEX i = 0; i < H; ++i) 
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
        for (TYPE_INDEX j = 0; j < 2*evictMatSize; ++j)
        {
            ORAM.createShares(this->evictMatrix[H][j], matrixShares);
            for (int k = 0; k < NUM_SERVERS; k++) 
            {   
                this->sharedMatrix[k][H][j] = matrixShares[k];
            }
		}
        
        end = time_now;
        cout<< "	[ClientKaryS3ORAMO] Shared Matrix Created in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<< " ns"<<endl;
        exp_logs[6] = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
        
        // 9.3. send permutation matrices to servers
        start = time_now;
        for (int i = 0; i < NUM_SERVERS; i++)
        {
            for (TYPE_INDEX y = 0 ; y < H; y++)
            {
                memcpy(&evict_buffer_out[i][y*evictMatSize*sizeof(TYPE_DATA)], this->sharedMatrix[i][y], evictMatSize*sizeof(TYPE_DATA));
            }
            memcpy(&evict_buffer_out[i][H*evictMatSize*sizeof(TYPE_DATA)], this->sharedMatrix[i][H], 2*evictMatSize*sizeof(TYPE_DATA));
            
            memcpy(&evict_buffer_out[i][(H+2)*evictMatSize*sizeof(TYPE_DATA)], &numEvict, sizeof(TYPE_INDEX));
            
            thread_args[i] = struct_socket(i, evict_buffer_out[i], (H+2)*evictMatSize*sizeof(TYPE_DATA) + sizeof(TYPE_INDEX), NULL,0, CMD_SEND_EVICT,  NULL);
            pthread_create(&thread_sockets[i], NULL, &ClientKaryS3ORAMO::thread_socket_func, (void*)&thread_args[i]);
        }
        for (int i = 0; i < NUM_SERVERS; i++)
        {
            pthread_join(thread_sockets[i], NULL);
        }
        end = time_now;
        cout<< "	[ClientKaryS3ORAMO] Eviction DONE in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<< " ns"<<endl;
        
        exp_logs[8] = thread_max;
        thread_max = 0;
		
		cout << "================================================================" << endl;
		cout << "EVICTION-" << this->numEvict+1 << " COMPLETED" << endl;
		cout << "================================================================" << endl;
        
        
        cout<<pos_map[1].pathID<<endl;
        cout<<pos_map[1].pathIdx<<endl;
        cout<<ORAM.getEvictString(numEvict)<<endl;
        
        this->numEvict = (numEvict+1) % N_leaf;
        

    }
    // 11. store local info to disk
    saveState();

    if(blockID==1)
    {
        cout<<pos_map[blockID].pathID<<endl;
        cout<<pos_map[blockID].pathIdx<<endl;
    }
    
	// 12. write log
	Utils::write_list_to_file(to_string(HEIGHT)+"_" + to_string(BLOCK_SIZE)+"_client_" + timestamp + ".txt",logDir, exp_logs, 9);
	memset(exp_logs, 0, sizeof(unsigned long int)*9);
	
	return 0;
}





/**
 * Function Name: getLogicalVector
 *
 * Description: Generates logical retrieve vector by putting '1' for the exact index of 
 * accessed block and '0' for the rest on its assigned path
 * 
 * @param logicalVector: (output) Logical retrieve vector to retrive the block.
 * @param blockID: (input) ID of the block to be retrieved.
 * @return 0 if successful
 */  
int ClientKaryS3ORAMO::getLogicalVector(TYPE_DATA* logicalVector, TYPE_ID blockID)
{
	TYPE_INDEX loc = pos_map[blockID].pathIdx;
	memset (logicalVector,0,sizeof(TYPE_DATA)*(H+2)*BUCKET_SIZE);
    logicalVector[loc] = 1;
	
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
int ClientKaryS3ORAMO::getEvictMatrix()
{
	S3ORAM ORAM;
    
    string strEvict = ORAM.getEvictString(numEvict);
    
    TYPE_INDEX fullEvictPathIdx[H+1];
    ORAM.getFullEvictPathIdx(fullEvictPathIdx,strEvict);
    
    TYPE_INDEX fullBlockPathIdx[H+1];
    
	cout<< "	[ClientKaryS3ORAMO] Creating Evict Matrix..." << endl;
	boost::progress_display show_progress(H);
	
    for(int h = 0 ; h  < H; h++)
    {
        TYPE_INDEX currSrcIdx = fullEvictPathIdx[h];
        TYPE_INDEX childNodeIdx[K_ARY];
        
        int NextLevEmptyIdx[K_ARY] ={0}; 
        for(int i = 0 ; i < K_ARY; i++)
        {
            childNodeIdx[i] = currSrcIdx*K_ARY+(i+1);
        }
        
        //for each real blocks in source bucket
        for(int z = 0 ; z <BUCKET_SIZE ; z++)
        {
            if(metaData[currSrcIdx][z] !=0)
            {
                TYPE_ID BlockID = metaData[currSrcIdx][z];
                unsigned long long path_idx = pos_map[BlockID].pathIdx % BUCKET_SIZE;
               
                //get the pathID of BlockID;
                ORAM.getFullPathIdx(fullBlockPathIdx,pos_map[BlockID].pathID);
                TYPE_INDEX nextLevIdx = fullBlockPathIdx[h+1];
               
                //evictMatrix[h][path_idx][((nextLevIdx-1)%K_ARY)*K_ARY + NextLevEmptyIdx[(nextLevIdx-1)%K_ARY] ] = 1;
                evictMatrix[h][(((nextLevIdx-1)%K_ARY)*(BUCKET_SIZE/K_ARY) + NextLevEmptyIdx[(nextLevIdx-1)%K_ARY])*BUCKET_SIZE + path_idx ] = 1;
                
                int slideIdx = (strEvict[h]-'0');
                
                metaData[nextLevIdx][slideIdx*(BUCKET_SIZE/K_ARY) + NextLevEmptyIdx[(nextLevIdx-1)%K_ARY]] = BlockID;
                
                pos_map[BlockID].pathIdx = (h+1) * BUCKET_SIZE + slideIdx*(BUCKET_SIZE/K_ARY) + NextLevEmptyIdx[(nextLevIdx-1)%K_ARY];
            
                
                NextLevEmptyIdx[(nextLevIdx-1)%K_ARY]++;
                
                metaData[currSrcIdx][z] =0;
                
                if(NextLevEmptyIdx[(nextLevIdx-1)%K_ARY]==(BUCKET_SIZE/K_ARY)+1)
                {
                    cout<< "	[ClientKaryS3ORAMO] Overflow!!!!. Please check the random generator or select smaller evict_rate"<<endl;
                    exit(0);
                }
            }
        }
        for(int i = 0 ; i < K_ARY; i++)
        {
            cout<<"Used slots: "<<NextLevEmptyIdx[i]<<endl;
        }
		++show_progress;
    }
    
    //move real blocks in leaf nodes to auxiliary buckets
    TYPE_INDEX AuxBukIdx = fullEvictPathIdx[H] + N_leaf;
    
    //for each real blocks in auxiliary bucket, dont move
    vector<int> lstEmptyIdx;
    lstEmptyIdx.clear();
    for(int z = 0 ; z <BUCKET_SIZE ; z++)
    {
        if(metaData[AuxBukIdx][z] !=0)
        {
            TYPE_ID currBlockID = metaData[AuxBukIdx][z];
            int j = pos_map[currBlockID].pathIdx % BUCKET_SIZE;
            evictMatrix[H][j*(2*BUCKET_SIZE) + (j+BUCKET_SIZE)] = 1;
        }
        else
        {
            lstEmptyIdx.push_back(z);
        }        
    }
    
    //for each real block in leaf bucket, move it into auxiliary bucket
    for(int z = 0 ; z <BUCKET_SIZE ; z++)
    {
        if(metaData[fullEvictPathIdx[H]][z] !=0)
        {
            TYPE_ID currBlockID = metaData[fullEvictPathIdx[H]][z];
            int i = pos_map[currBlockID].pathIdx % BUCKET_SIZE;
            int j = lstEmptyIdx.back();
            lstEmptyIdx.pop_back();
            evictMatrix[H][j*(2*BUCKET_SIZE) + i] = 1;
            
            metaData[fullEvictPathIdx[H]][z] = 0;
            metaData[AuxBukIdx][j] = currBlockID; 
            pos_map[currBlockID].pathIdx = BUCKET_SIZE*(H+1) + j;
            
        }
    }
    
	return 0;
}
 
 