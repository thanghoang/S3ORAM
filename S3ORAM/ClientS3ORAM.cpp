/*
 * ClientS3ORAM.cpp
 *
 *  Created on: Mar 15, 2017
 *      Author: ceyhunozkaptan, thanghoang
 */

#include "ClientS3ORAM.hpp"
#include "Utils.hpp"
#include "S3ORAM.hpp"


unsigned long int ClientS3ORAM::exp_logs[9];
unsigned long int ClientS3ORAM::thread_max = 0;
char ClientS3ORAM::timestamp[16];

ClientS3ORAM::ClientS3ORAM()
{
	this->pos_map = new TYPE_POS_MAP[NUM_BLOCK+1];
    
    this->metaData = new TYPE_ID*[NUM_NODES];
	for (int i = 0 ; i < NUM_NODES; i++)
    {
        this->metaData[i] = new TYPE_ID[BUCKET_SIZE];
    }
    
    retrievedShare = new TYPE_DATA*[NUM_SERVERS];
	for(int k = 0 ; k < NUM_SERVERS; k++)
    {
        retrievedShare[k] = new TYPE_DATA[DATA_CHUNKS];
    }
    recoveredBlock = new TYPE_DATA[DATA_CHUNKS];
    
    blocks_buffer_in = new unsigned char*[NUM_SERVERS];
    
    for(int i = 0; i < NUM_SERVERS ; i++)
    {
        blocks_buffer_in[i] = new unsigned char[sizeof(TYPE_DATA)*DATA_CHUNKS];
    }
    
	this->sharedMatrix = new TYPE_DATA**[NUM_SERVERS];
	for (TYPE_INDEX i = 0 ; i < NUM_SERVERS; i++)
	{
		this->sharedMatrix[i] = new TYPE_DATA*[H+1];
		for(TYPE_INDEX j = 0 ; j < H+1; j++)
		{
			this->sharedMatrix[i][j] = new TYPE_DATA[evictMatSize];
		}
	}

	this->evictMatrix = new TYPE_DATA*[H+1];
	for(TYPE_INDEX i = 0 ; i < H+1; i++)
	{
		this->evictMatrix[i] = new TYPE_DATA[evictMatSize];
	}
	
	this->sharedVector = new TYPE_DATA*[NUM_SERVERS];
	for(int i = 0; i < NUM_SERVERS; i++)
	{
		this->sharedVector[i] = new TYPE_DATA[(H+1)*BUCKET_SIZE];
	}
	
    this->vector_buffer_out = new unsigned char*[NUM_SERVERS];
    for (TYPE_INDEX i = 0 ; i < NUM_SERVERS ; i++)
    {
        this->vector_buffer_out[i] = new unsigned char[sizeof(TYPE_INDEX)+(H+1)*BUCKET_SIZE*sizeof(TYPE_DATA)]; 
    }
    
    this->block_buffer_out = new unsigned char*[NUM_SERVERS];
    for (int i = 0 ; i < NUM_SERVERS; i ++)
    {
        this->block_buffer_out[i]= new unsigned char[sizeof(TYPE_DATA)*DATA_CHUNKS+sizeof(TYPE_INDEX)];
        memset(this->block_buffer_out[i], 0, sizeof(TYPE_DATA)*DATA_CHUNKS+sizeof(TYPE_INDEX) );
    }
        
    this->evict_buffer_out = new unsigned char*[NUM_SERVERS];
    for (TYPE_INDEX i = 0 ; i < NUM_SERVERS ; i++)
    {
        this->evict_buffer_out[i] = new unsigned char[(H+1)*evictMatSize*sizeof(TYPE_DATA) + sizeof(TYPE_INDEX)];
    }
    
	#if defined(PRECOMP_MODE) // ================================================================================================
		this->precompOnes = new TYPE_DATA*[NUM_SERVERS];
		for (TYPE_INDEX i = 0 ; i < NUM_SERVERS ; i++){
			this->precompOnes[i] = new TYPE_DATA[PRECOMP_SIZE];
		}
		
		this->precompZeros = new TYPE_DATA*[NUM_SERVERS];
		for (TYPE_INDEX i = 0 ; i < NUM_SERVERS ; i++){
			this->precompZeros[i] = new TYPE_DATA[PRECOMP_SIZE];
		}
		
		S3ORAM ORAM;
		
		auto start = time_now;
		ORAM.precomputeShares(0, precompZeros, PRECOMP_SIZE);
		ORAM.precomputeShares(1, precompOnes, PRECOMP_SIZE);
		auto end = time_now;
		cout<< "	[ClientS3ORAM] " << 2*PRECOMP_SIZE << " Logical Values Precomputed in" << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<< " ns"<<endl;
	#endif //defined(PRECOMP_MODE) ================================================================================================
	
	time_t now = time(0);
	char* dt = ctime(&now);
	FILE* file_out = NULL;
	string path = clientLocalDir + "lastest_config";
	string info = "Height of Tree: " + to_string(HEIGHT) + "\n";
	info += "Number of Blocks: " + to_string(NUM_BLOCK) + "\n";
	info += "Bucket Size: " + to_string(BUCKET_SIZE) + "\n";
	info += "Eviction Rate: " + to_string(EVICT_RATE) + "\n";
	info += "Block Size (B): " + to_string(BLOCK_SIZE) + "\n";
	info += "ID Size (B): " + to_string(sizeof(TYPE_ID)) + "\n";
	info += "Number of Chunks: " + to_string(DATA_CHUNKS) + "\n";
	info += "Total Size of Data (MB): " + to_string((NUM_BLOCK*(BLOCK_SIZE+sizeof(TYPE_ID)))/1048576.0) + "\n";
	info += "Total Size of ORAM (MB): " + to_string(BUCKET_SIZE*NUM_NODES*(BLOCK_SIZE+sizeof(TYPE_ID))/1048576.0) + "\n";
	
	#if defined(PRECOMP_MODE)
		info += "PRECOMPUTATION MODE: Active\n";
	#else
		info += "PRECOMPUTATION MODE: Inactive\n";
	#endif 
	
	if((file_out = fopen(path.c_str(),"w+")) == NULL){
		cout<< "	File Cannot be Opened!!" <<endl;
		exit;
	}
	fputs(dt, file_out);
	fputs(info.c_str(), file_out);
	fclose(file_out);
	
	tm *now_time = localtime(&now);
	if(now != -1)
		strftime(timestamp,16,"%d%m_%H%M",now_time);
		
}

ClientS3ORAM::~ClientS3ORAM()
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
int ClientS3ORAM::init()
{
    this->numRead = 0;
    this->numEvict = 0;

    auto start = time_now;
    auto end = time_now;

    for ( TYPE_INDEX i = 0 ; i <= NUM_BLOCK; i ++ )
    {
        this->pos_map[i].pathID = -1;
        this->pos_map[i].pathIdx = -1;
    }

    start = time_now;
    S3ORAM ORAM;
    ORAM.build(this->pos_map,this->metaData);
    
    end = time_now;
	cout<<endl;
    cout<< "Elapsed Time for Setup on Disk: "<<std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<<" ns"<<endl;
    cout<<endl;
    std::ofstream output;
    string path2 = clientLocalDir + "lastest_config";
    output.open(path2, std::ios_base::app);
    output<< "INITIALIZATION ON CLIENT: Performed\n";
    output.close();

    //transfer to servers all ORAM structure (send files) //IMPLEMENT LATER
	
	FILE* local_data = NULL;
	if((local_data = fopen(clientTempPath.c_str(),"wb+")) == NULL){
		cout<< "	[init] File Cannot be Opened!!" <<endl;
		exit(0);
	}
	fwrite(this->pos_map, 1, (NUM_BLOCK+1)*sizeof(TYPE_POS_MAP), local_data);
	fwrite(&this->numEvict, sizeof(this->numEvict), 1, local_data);
	fwrite(&this->numRead, sizeof(this->numRead), 1, local_data);
	fclose(local_data);
	
    return 0;
}


/**
 * Function Name: load
 *
 * Description: Loads client storage data from disk for previously generated ORAM structure 
 * in order to continue ORAM operations. Loaded data includes postion map, current number of evictions,
 * current number of reads/writes.
 * 
 * @return 0 if successful
 */ 
int ClientS3ORAM::load()
{
	FILE* local_data = NULL;
	if((local_data = fopen(clientTempPath.c_str(),"rb")) == NULL){
		cout<< "	[load] File Cannot be Opened!!" <<endl;
		exit(0);
	}
	
	long lSize;
	fseek (local_data , 0 , SEEK_END);
	lSize = ftell (local_data);
	rewind (local_data);
	
	unsigned char* local_data_buffer = new unsigned char[sizeof(char)*lSize];
	if(fread(local_data_buffer ,1 , sizeof(char)*lSize, local_data) != sizeof(char)*lSize){
		cout<< "	[load] File Cannot be Read!!" <<endl;
		exit(0);
	}
	fclose(local_data);
	
	size_t currSize = 0;
	memcpy(this->pos_map, &local_data_buffer[currSize], (NUM_BLOCK+1)*sizeof(TYPE_POS_MAP));
	currSize += (NUM_BLOCK+1)*sizeof(TYPE_POS_MAP);
	memcpy(&this->numEvict, &local_data_buffer[currSize], sizeof(this->numEvict));
	cout << "[load] Eviction number: "<<numEvict << endl;
	currSize += sizeof(this->numEvict);
	memcpy(&this->numRead, &local_data_buffer[currSize], sizeof(this->numRead));
	cout << "[load] Retrieval number: "<<numRead << endl;
	
	//scan position map to load bucket metaData (for speed optimization)
    TYPE_INDEX fullPathIdx[H+1];
    S3ORAM ORAM;
    for(TYPE_INDEX i = 1 ; i < NUM_BLOCK+1; i++)
    {
        ORAM.getFullPathIdx(fullPathIdx,pos_map[i].pathID);
        this->metaData[fullPathIdx[pos_map[i].pathIdx/ BUCKET_SIZE]][pos_map[i].pathIdx%BUCKET_SIZE] = i;
    }
    
	std::ofstream output;
	string path = clientLocalDir + "lastest_config";
	output.open(path, std::ios_base::app);
	output<< "SETUP FROM LOCAL DATA\n";
	output.close();
	
	delete local_data_buffer;
	
    return 0;
}


/**
 * Function Name: sendORAMTree
 *
 * Description: Distributes generated and shared ORAM buckets to servers over network
 * 
 * @return 0 if successful
 */  
int ClientS3ORAM::sendORAMTree()
{
    unsigned char*  oram_buffer_out = new unsigned char [BUCKET_SIZE*sizeof(TYPE_DATA)*DATA_CHUNKS]; 
    memset(oram_buffer_out,0, BUCKET_SIZE*sizeof(TYPE_DATA)*DATA_CHUNKS);
    int CMD = CMD_SEND_ORAM_TREE;       
    unsigned char buffer_in[sizeof(CMD_SUCCESS)];
	unsigned char buffer_out[sizeof(CMD)];

    memcpy(buffer_out, &CMD,sizeof(CMD));
    
    zmq::context_t context(1);
    zmq::socket_t socket(context,ZMQ_REQ);

    struct_socket thread_args[NUM_SERVERS];
    for (int i = 0; i < NUM_SERVERS; i++)
    {
//        string ADDR = SERVER_ADDR[i]+ ":" + SERVER_PORT[i*NUM_SERVERS+i]; 
		string ADDR = SERVER_ADDR[i]+ ":" + std::to_string(SERVER_PORT+i*NUM_SERVERS+i); 
		cout<< "	[sendORAMTree] Connecting to " << ADDR <<endl;
        socket.connect( ADDR.c_str());
            
        socket.send(buffer_out, sizeof(CMD));
		cout<< "	[sendORAMTree] Command SENT! " << CMD <<endl;
        socket.recv(buffer_in, sizeof(CMD_SUCCESS));
		
        for(TYPE_INDEX j = 0 ; j < NUM_NODES; j++)
        {
            //load data to buffer
            FILE* fdata = NULL;
            string path = rootPath + to_string(i) + "/" + to_string(j);
            if((fdata = fopen(path.c_str(),"rb")) == NULL)
            {
                cout<< "	[sendORAMTree] File Cannot be Opened!!" <<endl;
                exit(0);
            }
            long lSize;
            fseek (fdata , 0 , SEEK_END);
            lSize = ftell (fdata);
            rewind (fdata);
            if(fread(oram_buffer_out ,1 , BUCKET_SIZE*sizeof(TYPE_DATA)*DATA_CHUNKS, fdata) != sizeof(char)*lSize){
                cout<< "	[sendORAMTree] File loading error be Read!!" <<endl;
                exit(0);
            }
            fclose(fdata);
            //send to server i
            socket.send(oram_buffer_out,BUCKET_SIZE*sizeof(TYPE_DATA)*DATA_CHUNKS,0);
            socket.recv(buffer_in,sizeof(CMD_SUCCESS));
        }
        socket.disconnect(ADDR.c_str());
    }
    socket.close();	
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
int ClientS3ORAM::access(TYPE_ID blockID)
{
    S3ORAM ORAM;
	cout << "================================================================" << endl;
	cout << "STARTING ACCESS OPERATION FOR BLOCK-" << blockID <<endl; 
	cout << "================================================================" << endl;
	
    // 1. get the path & index of the block of interest
    TYPE_INDEX pathID = pos_map[blockID].pathID;
	cout << "	[ClientS3ORAM] PathID = " << pathID <<endl;
    cout << "	[ClientS3ORAM] Location = " << 	pos_map[blockID].pathIdx <<endl;
    
    // 2. create select query
	TYPE_DATA logicVector[(H+1)*BUCKET_SIZE];
	
	auto start = time_now;
    auto end = time_now;
	
	start = time_now;
	getLogicalVector(logicVector, blockID);
	end = time_now;
	cout<< "	[ClientS3ORAM] Logical Vector Created in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<< " ns"<<endl;
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
		cout<< "	[ClientS3ORAM] Shared Vector Created in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<< " ns"<<endl;
		exp_logs[1] = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
	#else //defined (PRECOMP_MODE) ================================================================================================
		start = time_now;
		ORAM.getSharedVector(logicVector, this->sharedVector);
		end = time_now;
		cout<< "	[ClientS3ORAM] Shared Vector Created in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<< " ns"<<endl;
		exp_logs[1] = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
	#endif //defined (PRECOMP_MODE) ================================================================================================
	
    
    
	// 4. send to server & receive the answer
    struct_socket thread_args[NUM_SERVERS];
    
    start = time_now;
    for (int i = 0; i < NUM_SERVERS; i++)
    {
        memcpy(&vector_buffer_out[i][0], &pathID, sizeof(pathID));
        memcpy(&vector_buffer_out[i][sizeof(pathID)], &this->sharedVector[i][0], (H+1)*BUCKET_SIZE*sizeof(TYPE_DATA));
        
//        thread_args[i] = struct_socket(SERVER_ADDR[i]+ ":" + SERVER_PORT[i*NUM_SERVERS+i], vector_buffer_out[i], sizeof(pathID)+(H+1)*BUCKET_SIZE*sizeof(TYPE_DATA), blocks_buffer_in[i], sizeof(TYPE_DATA)*DATA_CHUNKS,CMD_REQUEST_BLOCK,NULL);
		thread_args[i] = struct_socket(SERVER_ADDR[i]+ ":" + std::to_string(SERVER_PORT+i*NUM_SERVERS+i), vector_buffer_out[i], sizeof(pathID)+(H+1)*BUCKET_SIZE*sizeof(TYPE_DATA), blocks_buffer_in[i], sizeof(TYPE_DATA)*DATA_CHUNKS,CMD_REQUEST_BLOCK,NULL);

		pthread_create(&thread_sockets[i], NULL, &ClientS3ORAM::thread_socket_func, (void*)&thread_args[i]);
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
        cout << "	[ClientS3ORAM] From Server-" << i+1 << " => BlockID = " << retrievedShare[i][0]<< endl;
    }
    end = time_now;
    cout<< "	[ClientS3ORAM] All Shares Retrieved in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<< " ns"<<endl;
    exp_logs[2] = thread_max;
    thread_max = 0;
	
    // 5. recover the block
	start = time_now;
	ORAM.simpleRecover(retrievedShare, recoveredBlock);
	end = time_now;
	cout<< "	[ClientS3ORAM] Recovery Done in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<< " ns"<<endl;
	exp_logs[3] = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
	
    
    cout << "	[ClientS3ORAM] Block-" << recoveredBlock[0] <<" is Retrieved" <<endl;
    if (recoveredBlock[0] == blockID)
        cout << "	[ClientS3ORAM] SUCCESS!!!!!!" << endl;
    else
        cout << "	[ClientS3ORAM] ERROR!!!!!!!!" << endl;
		
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
		thread_args[k] = struct_socket(SERVER_ADDR[k]+ ":" + std::to_string(SERVER_PORT+k*NUM_SERVERS+k), block_buffer_out[k], sizeof(TYPE_DATA)*DATA_CHUNKS+sizeof(TYPE_INDEX), NULL, 0, CMD_SEND_BLOCK,NULL);

		pthread_create(&thread_sockets[k], NULL, &ClientS3ORAM::thread_socket_func, (void*)&thread_args[k]);
    }
    
	this->numRead = (this->numRead+1)%EVICT_RATE;
    cout << "	[ClientS3ORAM] Number of Read = " << this->numRead <<endl;
    
    for (int i = 0; i < NUM_SERVERS; i++)
    {
        pthread_join(thread_sockets[i], NULL);
        cout << "	[ClientS3ORAM] Block upload completed!" << endl;
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
        this->getEvictMatrix(this->evictMatrix,numEvict);
		end = time_now;
		cout<< "	[ClientS3ORAM] Evict Matrix Created in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<< " ns"<<endl;
		exp_logs[5] = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
		
		
		// 9.2. create shares of  permutation matrices 
		cout<< "	[ClientS3ORAM] Sharing Evict Matrix..." << endl;
		
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
        cout<< "	[ClientS3ORAM] Shared Matrix Created in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<< " ns"<<endl;
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
            
			thread_args[i] = struct_socket(SERVER_ADDR[i]+ ":" + std::to_string(SERVER_PORT+i*NUM_SERVERS+i), evict_buffer_out[i], (H+1)*evictMatSize*sizeof(TYPE_DATA) + sizeof(TYPE_INDEX), NULL,0, CMD_SEND_EVICT,  NULL);
//            thread_args[i] = struct_socket(SERVER_ADDR[i]+ ":" + SERVER_PORT[i*NUM_SERVERS+i], evict_buffer_out[i], (H+1)*evictMatSize*sizeof(TYPE_DATA) + sizeof(TYPE_INDEX), NULL,0, CMD_SEND_EVICT,  NULL);
            pthread_create(&thread_sockets[i], NULL, &ClientS3ORAM::thread_socket_func, (void*)&thread_args[i]);
        }
			
        for (int i = 0; i < NUM_SERVERS; i++)
        {
            pthread_join(thread_sockets[i], NULL);
        }
        end = time_now;
        cout<< "	[ClientS3ORAM] Eviction DONE in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<< " ns"<<endl;
        
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
		cout<< "	[ClientS3ORAM] File Cannot be Opened!!" <<endl;
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
 * Function Name: getLogicalVector
 *
 * Description: Generates logical retrieve vector by putting '1' for the exact index of 
 * accessed block and '0' for the rest on its assigned path
 * 
 * @param logicalVector: (output) Logical retrieve vector to retrive the block.
 * @param blockID: (input) ID of the block to be retrieved.
 * @return 0 if successful
 */  
int ClientS3ORAM::getLogicalVector(TYPE_DATA* logicalVector, TYPE_ID blockID)
{
	TYPE_INDEX loc = pos_map[blockID].pathIdx;
	memset (logicalVector,0,sizeof(TYPE_DATA)*(H+1)*BUCKET_SIZE);
    logicalVector[loc] = 1;
	
	return 0;
}


/**
 * Function Name: getEvictMatrix
 *
 * Description: Generates logical eviction matrix to evict blocks from root to leaves according to 
 * eviction number and source, destination and sibling buckets by scanning position map.
 * 
 * @param evictMatrix: (output) Logical eviction matrix for eviction routine
 * @param n_evict: (input) Eviction number
 * @return 0 if successful
 */  
int ClientS3ORAM::getEvictMatrix(TYPE_DATA** evictMatrix, TYPE_INDEX n_evict)
{
	S3ORAM ORAM;
    TYPE_INDEX* src_idx = new TYPE_INDEX[H];
    TYPE_INDEX* dest_idx = new TYPE_INDEX[H];
    TYPE_INDEX* sibl_idx = new TYPE_INDEX[H];
    
    string evict_str = ORAM.getEvictString( n_evict);
    
    ORAM.getEvictIdx(src_idx,dest_idx,sibl_idx,evict_str);
	cout<< "	[ClientS3ORAM] Creating Evict Matrix..." << endl;
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
                        cout<< "	[ClientS3ORAM] Overflow!!!!. Please check the random generator or select smaller evict_rate"<<endl;
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
                            cout<< "	[ClientS3ORAM] Overflow!!!, Please check the random generator or select smaller evict_rate"<<endl;
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
 
 
/**
 * Function Name: thread_socket_func & send
 *
 * Description: Generic threaded socket function for send and receive operations
 * 
 * @return 0 if successful
 */  
void* ClientS3ORAM::thread_socket_func(void* args)
{
	struct_socket* opt = (struct_socket*) args;
	
	
	sendNrecv(opt->ADDR, opt->data_out, opt->data_out_size, opt->data_in, opt->data_in_size, opt->CMD);

		
    pthread_exit((void*)opt);
}
int ClientS3ORAM::sendNrecv(std::string ADDR, unsigned char* data_out, size_t data_out_size, unsigned char* data_in, size_t data_in_size, int CMD)
{
	zmq::context_t context(1);
    zmq::socket_t socket(context,ZMQ_REQ);
    socket.connect(ADDR.c_str());
	
    unsigned char buffer_in[sizeof(CMD_SUCCESS)];
	unsigned char buffer_out[sizeof(CMD)];
	
    try
    {
        cout<< "	[ThreadSocket] Sending Command to"<< ADDR << endl;
        memcpy(buffer_out, &CMD,sizeof(CMD));
        socket.send(buffer_out, sizeof(CMD));
		cout<< "	[ThreadSocket] Command SENT! " << CMD <<endl;
        socket.recv(buffer_in, sizeof(CMD_SUCCESS));
		
		auto start = time_now;
		cout<< "	[ThreadSocket] Sending Data..." << endl;
		socket.send (data_out, data_out_size);
		cout<< "	[ThreadSocket] Data SENT!" << endl;
        if(data_in_size == 0)
            socket.recv(buffer_in,sizeof(CMD_SUCCESS));
        else
            socket.recv(data_in,data_in_size);
            
		auto end = time_now;
		if(thread_max < std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count())
			thread_max = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
	}
    catch (exception &ex)
    {
        cout<< "	[ThreadSocket] Socket error!"<<endl;
		exit(0);
    }
	socket.disconnect(ADDR.c_str());
	return 0;
}
