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




zmq::context_t** ClientS3ORAM::context = new zmq::context_t*[NUM_SERVERS];
zmq::socket_t**  ClientS3ORAM::socket = new zmq::socket_t*[NUM_SERVERS];
    




ClientS3ORAM::ClientS3ORAM()
{
    //specific
    this->sharedMatrix = new TYPE_DATA**[NUM_SERVERS];
	for (TYPE_INDEX i = 0 ; i < NUM_SERVERS; i++)
	{
		this->sharedMatrix[i] = new TYPE_DATA*[H+1];
		for(TYPE_INDEX j = 0 ; j < H; j++)
		{
			this->sharedMatrix[i][j] = new TYPE_DATA[evictMatSize];
		}
        this->sharedMatrix[i][H] = new TYPE_DATA[2*evictMatSize];
		
	}
    //for(int i = 0 ; i < 2*BUCKET_SIZE; i++)
    //{
//        memset(auxEvictMatrix,0,2*BUCKET_SIZE*BUCKET_SIZE*sizeof(TYPE_DATA));
    //}

	this->evictMatrix = new TYPE_DATA*[H+1];
	for(TYPE_INDEX i = 0 ; i < H; i++)
	{
		this->evictMatrix[i] = new TYPE_DATA[evictMatSize];
    }
    this->evictMatrix[H] = new TYPE_DATA[2*evictMatSize];
    
    
    this->block_buffer_out = new unsigned char*[NUM_SERVERS];
    for (int i = 0 ; i < NUM_SERVERS; i ++)
    {
        this->block_buffer_out[i]= new unsigned char[sizeof(TYPE_DATA)*DATA_CHUNKS+sizeof(TYPE_INDEX)];
        memset(this->block_buffer_out[i], 0, sizeof(TYPE_DATA)*DATA_CHUNKS+sizeof(TYPE_INDEX) );
    }
        
    this->evict_buffer_out = new unsigned char*[NUM_SERVERS];
    for (TYPE_INDEX i = 0 ; i < NUM_SERVERS ; i++)
    {
        this->evict_buffer_out[i] = new unsigned char[(H+2)*evictMatSize*sizeof(TYPE_DATA) + sizeof(TYPE_INDEX)];
    }
    
    
    
    
    //H+2, instead of H+1;
	this->sharedVector = new TYPE_DATA*[NUM_SERVERS];
	for(int i = 0; i < NUM_SERVERS; i++)
	{
		this->sharedVector[i] = new TYPE_DATA[(H+2)*BUCKET_SIZE];
	}
	
    this->vector_buffer_out = new unsigned char*[NUM_SERVERS];
    for (TYPE_INDEX i = 0 ; i < NUM_SERVERS ; i++)
    {
        this->vector_buffer_out[i] = new unsigned char[sizeof(TYPE_INDEX)+(H+2)*BUCKET_SIZE*sizeof(TYPE_DATA)]; 
    }
    
    
    //inherent
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
	
    
    
    //socket
    for(int i = 0 ; i < NUM_SERVERS;i ++)
    {
        context[i] = new zmq::context_t(1);
        socket[i] = new zmq::socket_t(*context[i],ZMQ_REQ);
        string send_address = SERVER_ADDR[i]+ ":" + std::to_string(SERVER_PORT+i*NUM_SERVERS+i);
        cout<<"Connecting to "<<send_address<<" for communication with Server "<< i << " ...";
        socket[i]->connect(send_address);
        cout<<"OK!"<<endl;
    }
    
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

    for ( TYPE_INDEX i = 0 ; i <= NUM_BLOCK; i++ )
    {
        this->pos_map[i].pathID = -1;
        this->pos_map[i].pathIdx = -1;
    }

    start = time_now;
    S3ORAM ORAM;
    ORAM.build(this->pos_map,this->metaData);
    
    //at the begining all blocks are in the auxiliary buckets, so, change the pos_map to be correct
    for(unsigned long long i = 1 ; i < NUM_BLOCK+1; i++)
    {
        this->pos_map[i].pathIdx += BUCKET_SIZE;
        this->pos_map[i].pathID -= N_leaf;
    }
    
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

    //save state
	saveState();
	
    return 0;
}

/**
 * Function Name: load (inherent+specific)
 *
 * Description: Loads client storage data from disk for previously generated ORAM structure 
 * in order to continue ORAM operations. Loaded data includes postion map, current number of evictions,
 * current number of reads/writes.
 * 
 * @return 0 if successful
 */ 
int ClientS3ORAM::loadState()
{
	FILE* local_data = NULL;
	if((local_data = fopen(clientTempPath.c_str(),"rb")) == NULL){
		cout<< "	[load] File Cannot be Opened!!" <<endl;
		exit(0);
	}
	
    fread(this->pos_map, 1, (NUM_BLOCK+1)*sizeof(TYPE_POS_MAP), local_data);
	fread(&this->numEvict, sizeof(this->numEvict), 1, local_data);
	fread(&this->numRead, sizeof(this->numRead), 1, local_data);
    
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
	
    return 0;
}



int ClientS3ORAM::saveState()
{
    // 11. store local info to disk
	FILE* local_data = NULL;
	if((local_data = fopen(clientTempPath.c_str(),"wb+")) == NULL){
		cout<< "	[ClientS3CORAM] File Cannot be Opened!!" <<endl;
		exit(0);
	}
	fwrite(this->pos_map, 1, (NUM_BLOCK+1)*sizeof(TYPE_POS_MAP), local_data);
	fwrite(&this->numEvict, sizeof(this->numEvict), 1, local_data);
	fwrite(&this->numRead, sizeof(this->numRead), 1, local_data);

	fclose(local_data);
    
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
	TYPE_DATA logicVector[(H+2)*BUCKET_SIZE];
	
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
		cout<< "	[ClientS3ORAM] Shared Vector Created in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<< " ns"<<endl;
		exp_logs[1] = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
	#else //defined (PRECOMP_MODE) ================================================================================================
		start = time_now;
		ORAM.getSharedVector(logicVector, this->sharedVector, (H+2)*BUCKET_SIZE);
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
        memcpy(&vector_buffer_out[i][sizeof(pathID)], &this->sharedVector[i][0], (H+2)*BUCKET_SIZE*sizeof(TYPE_DATA));
     	thread_args[i] = struct_socket(i, vector_buffer_out[i], sizeof(pathID)+(H+2)*BUCKET_SIZE*sizeof(TYPE_DATA), blocks_buffer_in[i], sizeof(TYPE_DATA)*DATA_CHUNKS,CMD_REQUEST_BLOCK,NULL);

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
        pthread_create(&thread_sockets[k], NULL, &ClientS3ORAM::thread_socket_func, (void*)&thread_args[k]);
    }
    for (int i = 0; i < NUM_SERVERS; i++)
    {
        pthread_join(thread_sockets[i], NULL);
        cout << "	[ClientS3ORAM] Block upload completed!" << endl;
    }
    
    
	cout << "================================================================" << endl;
	cout << "ACCESS OPERATION FOR BLOCK-" << blockID << " COMPLETED." << endl; 
	cout << "================================================================" << endl;
	
    this->numRead = (this->numRead+1)%EVICT_RATE;
    cout << "	[ClientS3ORAM] Number of Read = " << this->numRead <<endl;
    
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
        this->getEvictMatrix(numEvict);
		end = time_now;
		
        cout<< "	[ClientS3ORAM] Evict Matrix Created in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<< " ns"<<endl;
		exp_logs[5] = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
		
		// 9.2. create shares of  permutation matrices 
		cout<< "	[ClientS3ORAM] Sharing Evict Matrix..." << endl;
		
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
        cout<< "	[ClientS3ORAM] Shared Matrix Created in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<< " ns"<<endl;
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
int ClientS3ORAM::getLogicalVector(TYPE_DATA* logicalVector, TYPE_ID blockID)
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
 * @param evictMatrix: (output) Logical eviction matrix for eviction routine
 * @param n_evict: (input) Eviction number
 * @return 0 if successful
 */  
int ClientS3ORAM::getEvictMatrix(TYPE_INDEX n_evict)
{
	S3ORAM ORAM;
    
    string strEvict = ORAM.getEvictString( n_evict);
    
    TYPE_INDEX fullEvictPathIdx[H+1];
    ORAM.getFullEvictPathIdx(fullEvictPathIdx,strEvict);
    
    TYPE_INDEX fullBlockPathIdx[H+1];
    
	cout<< "	[ClientS3ORAM] Creating Evict Matrix..." << endl;
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
                    cout<< "	[ClientS3ORAM] Overflow!!!!. Please check the random generator or select smaller evict_rate"<<endl;
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
	
	
	sendNrecv(opt->peer_idx, opt->data_out, opt->data_out_size, opt->data_in, opt->data_in_size, opt->CMD);

		
    pthread_exit((void*)opt);
}
int ClientS3ORAM::sendNrecv(int peer_idx, unsigned char* data_out, size_t data_out_size, unsigned char* data_in, size_t data_in_size, int CMD)
{

    unsigned char buffer_in[sizeof(CMD_SUCCESS)];
	unsigned char buffer_out[sizeof(CMD)];
	
    try
    {
        cout<< "	[ThreadSocket] Sending Command to"<< SERVER_ADDR[peer_idx] << endl;
        memcpy(buffer_out, &CMD,sizeof(CMD));
        socket[peer_idx]->send(buffer_out, sizeof(CMD));
		cout<< "	[ThreadSocket] Command SENT! " << CMD <<endl;
        socket[peer_idx]->recv(buffer_in, sizeof(CMD_SUCCESS));
		
		auto start = time_now;
		cout<< "	[ThreadSocket] Sending Data..." << endl;
		socket[peer_idx]->send (data_out, data_out_size);
		cout<< "	[ThreadSocket] Data SENT!" << endl;
        if(data_in_size == 0)
            socket[peer_idx]->recv(buffer_in,sizeof(CMD_SUCCESS));
        else
            socket[peer_idx]->recv(data_in,data_in_size);
            
		auto end = time_now;
		if(thread_max < std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count())
			thread_max = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
	}
    catch (exception &ex)
    {
        cout<< "	[ThreadSocket] Socket error!"<<endl;
		exit(0);
    }
	return 0;
}
