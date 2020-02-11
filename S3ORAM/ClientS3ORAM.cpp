#include "ClientS3ORAM.hpp"
#include "S3ORAM.hpp"

unsigned long int ClientS3ORAM::exp_logs[9];
unsigned long int ClientS3ORAM::thread_max = 0;

char ClientS3ORAM::timestamp[16];


zmq::context_t** ClientS3ORAM::context = new zmq::context_t*[NUM_SERVERS];
zmq::socket_t**  ClientS3ORAM::socket = new zmq::socket_t*[NUM_SERVERS];

ClientS3ORAM::ClientS3ORAM()
{
    this->pos_map = new TYPE_POS_MAP[NUM_BLOCK+1];
    
    this->metaData = new TYPE_ID*[NUM_NODES];
	for (int i = 0 ; i < NUM_NODES; i++)
    {
        this->metaData[i] = new TYPE_ID[BUCKET_SIZE];
        memset(this->metaData[i],0,sizeof(TYPE_ID)*BUCKET_SIZE);
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
		cout<< "	[ClientKaryS3ORAMO] " << 2*PRECOMP_SIZE << " Logical Values Precomputed in" << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()<< " ns"<<endl;
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
 * Function Name: load 
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
