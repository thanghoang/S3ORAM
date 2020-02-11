#include "ServerS3ORAM.hpp"
#include "Utils.hpp"
#include "struct_socket.h"

#include "S3ORAM.hpp"
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>

#include "struct_thread_computation.h"
#include "struct_thread_loadData.h"

zmq::context_t** ServerS3ORAM::context_send = new zmq::context_t*[NUM_SERVERS-1];
zmq::socket_t**  ServerS3ORAM::socket_send = new zmq::socket_t*[NUM_SERVERS-1];
    

zmq::context_t** ServerS3ORAM::context_recv = new zmq::context_t*[NUM_SERVERS-1];
zmq::socket_t** ServerS3ORAM::socket_recv = new zmq::socket_t*[NUM_SERVERS-1];



unsigned long int ServerS3ORAM::server_logs[13];
unsigned long int ServerS3ORAM::thread_max = 0;
char ServerS3ORAM::timestamp[16];



ServerS3ORAM::ServerS3ORAM()
{
}

ServerS3ORAM::~ServerS3ORAM()
{
}



ServerS3ORAM::ServerS3ORAM(TYPE_INDEX serverNo, int selectedThreads) 
{
	
	this->CLIENT_ADDR = "tcp://*:" + std::to_string(SERVER_PORT+(serverNo)*NUM_SERVERS+serverNo);
    
    this->thread_compute = new pthread_t[numThreads];
	
    this->numThreads = selectedThreads;
    
	cout<<endl;
	cout << "=================================================================" << endl;
	cout<< "Starting Server-" << serverNo+1 <<endl;
	cout << "=================================================================" << endl;
	this->serverNo = serverNo;
	
	TYPE_INDEX m = 0;
	for (TYPE_INDEX k = 0 ; k < NUM_SERVERS; k++)
	{
		if(k != serverNo)
		{
			this->others[m] = k;
			m++;
		}
	}
    
    
    sumBlock = new TYPE_DATA[DATA_CHUNKS];
	this->BUCKET_DATA = new TYPE_DATA*[DATA_CHUNKS];
		
    for (TYPE_INDEX y = 0 ; y < DATA_CHUNKS ; y++)
    {
        this->BUCKET_DATA[y] = new TYPE_DATA[BUCKET_SIZE];
    }
	
	this->select_buffer_in = new unsigned char[sizeof(TYPE_INDEX)+(H+1)*BUCKET_SIZE*sizeof(TYPE_DATA)];
	this->block_buffer_out = new unsigned char[sizeof(TYPE_DATA)*DATA_CHUNKS];

	this->evict_buffer_in = new unsigned char[(H+1)*evictMatSize*sizeof(TYPE_DATA) + sizeof(TYPE_INDEX)];
	
	this->evictMatrix = new zz_p**[H+1];
	for(TYPE_INDEX y = 0 ; y < H+1; y++)
	{
		this->evictMatrix[y] = new zz_p*[BUCKET_SIZE];
		for(TYPE_INDEX i = 0 ; i < BUCKET_SIZE; i++)
		{
			this->evictMatrix[y][i] = new zz_p[2*BUCKET_SIZE];
		}
		
	}
    bucket_buffer = new unsigned char[BUCKET_SIZE*sizeof(TYPE_DATA)*DATA_CHUNKS];
        
	this->ownShares = new TYPE_DATA**[NUM_SERVERS];
	for(TYPE_INDEX i = 0 ; i < NUM_SERVERS ;  i++)
	{
		this->ownShares[i] = new TYPE_DATA*[DATA_CHUNKS];
		for(TYPE_INDEX ii = 0 ; ii < DATA_CHUNKS ;  ii++)
		{
			this->ownShares[i][ii] = new TYPE_DATA[BUCKET_SIZE];
		}
	}
    

    this->block_buffer_in = new unsigned char[sizeof(TYPE_DATA)*DATA_CHUNKS+ sizeof(TYPE_INDEX)];
     
	this->shares_buffer_in = new unsigned char*[NUM_SERVERS-1];
	for (TYPE_INDEX k = 0 ; k < NUM_SERVERS-1; k++)
	{
		this->shares_buffer_in[k] = new unsigned char[BUCKET_SIZE*sizeof(TYPE_DATA)*DATA_CHUNKS];
	}

	this->shares_buffer_out = new unsigned char*[NUM_SERVERS-1];
	for (TYPE_INDEX k = 0 ; k < NUM_SERVERS-1; k++)
	{
		this->shares_buffer_out[k] = new unsigned char[BUCKET_SIZE*sizeof(TYPE_DATA)*DATA_CHUNKS];
	}

	this->dot_product_vector = new zz_p*[DATA_CHUNKS];
	for (TYPE_INDEX k = 0 ; k < DATA_CHUNKS; k++)
	{
		this->dot_product_vector[k] = new zz_p[BUCKET_SIZE*(H+1)];
	}
	
    this->cross_product_vector = new zz_p*[DATA_CHUNKS];
    for (TYPE_INDEX k = 0 ; k < DATA_CHUNKS; k++)
	{
		this->cross_product_vector[k]  = new zz_p[BUCKET_SIZE*2];	
	}
	
	time_t rawtime = time(0);
	tm *now = localtime(&rawtime);

	if(rawtime != -1)
		strftime(timestamp,16,"%d%m_%H%M",now);
    
    
     //socket
    for(int i = 0 ; i < NUM_SERVERS-1;i ++)
    {
        context_send[i] = new zmq::context_t(1);
        socket_send[i] = new zmq::socket_t(*context_send[i],ZMQ_REQ);
        string send_address = SERVER_ADDR[this->others[i]] + ":" + std::to_string(SERVER_PORT+this->others[i]*NUM_SERVERS+this->serverNo);
        cout<<"Opening "<<send_address<<" for sending...";
        socket_send[i]->connect(send_address);
        cout<<"OK!"<<endl;
                
        context_recv[i] = new zmq::context_t(2);
        socket_recv[i] = new zmq::socket_t(*context_recv[i],ZMQ_REP);
        string recv_address = "tcp://*:" + std::to_string(SERVER_PORT+(serverNo)*(NUM_SERVERS)+this->others[i]);
        cout<<"Opening "<<recv_address<<" for listening...";
        socket_recv[i]->bind(recv_address);
        cout<<"OK!"<<endl;
        
    }
		
}





/**
 * Function Name: start
 *
 * Description: Starts the server to wait for a command from the client. 
 * According to the command, server performs certain subroutines for distributed ORAM operations.
 * 
 * @return 0 if successful
 */ 
int ServerS3ORAM::start()
{
	int ret = 1;
	int CMD;
    unsigned char buffer[sizeof(CMD)];
    zmq::context_t context(1);
    zmq::socket_t socket(context,ZMQ_REP);
    
	cout<< "[Server] Socket is OPEN on " << this->CLIENT_ADDR << endl;
    socket.bind(this->CLIENT_ADDR.c_str());

	while (true) 
	{
		cout<< "[Server] Waiting for a Command..." <<endl;
        socket.recv(buffer,sizeof(CMD));
		
        memcpy(&CMD, buffer, sizeof(CMD));
		cout<< "[Server] Command RECEIVED!" <<endl;
		
        socket.send((unsigned char*)CMD_SUCCESS,sizeof(CMD_SUCCESS));
        
        switch(CMD)
        {
			case CMD_SEND_ORAM_TREE: //inherent
				cout<<endl;
				cout << "=================================================================" << endl;
				cout<< "[Server] Receiving ORAM Data..." <<endl;
				cout << "=================================================================" << endl;
				this->recvORAMTree(socket);
				cout << "=================================================================" << endl;
				cout<< "[Server] ORAM Data RECEIVED!" <<endl;
				cout << "=================================================================" << endl;
				cout<<endl;
				break;
			case CMD_REQUEST_BLOCK: //inherent
				cout<<endl;
				cout << "=================================================================" << endl;
				cout<< "[Server] Receiving Logical Vector..." <<endl;
				cout << "=================================================================" << endl;
				this->retrieve(socket);
				cout << "=================================================================" << endl;
				cout<< "[Server] Block Share SENT" <<endl;
				cout << "=================================================================" << endl;
				cout<<endl;
				break;
            case CMD_SEND_BLOCK: //specific
				cout<<endl;
            	cout << "=================================================================" << endl;
				cout<< "[Server] Receiving Block Data..." <<endl;
				cout << "=================================================================" << endl;
				this->recvBlock(socket);
				cout << "=================================================================" << endl;
				cout<< "[Server] Block Data RECEIVED!" <<endl;
				cout << "=================================================================" << endl;
				cout<<endl;
				break;
			case CMD_SEND_EVICT: //specific
				cout<<endl;
				cout << "=================================================================" << endl;
				cout<< "Receiving Eviction Matrix..." <<endl;
				cout << "=================================================================" << endl;
				this->evict(socket);
				cout << "=================================================================" << endl;
				cout<< "[Server] EVICTION and DEGREE REDUCTION DONE!" <<endl;
				cout << "=================================================================" << endl;
				cout<<endl;
				break;
			default:
				break;
		}
	}
	
	ret = 0;
    return ret;
}



/**
 * Function Name: recvBlock
 *
 * Description: Receives the share of previosly accessed block from the client 
 * with its new index number and stores it into root bucket for later eviction. 
 * 
 * @param socket: (input) ZeroMQ socket instance for communication with the client
 * @return 0 if successful
 */  
int ServerS3ORAM::recvBlock(zmq::socket_t& socket)
{
	cout<< "	[recvBlock] Receiving Block Data..." <<endl;
	auto start = time_now;
	socket.recv(block_buffer_in, sizeof(TYPE_DATA)*DATA_CHUNKS+sizeof(TYPE_INDEX), 0);
	auto end = time_now;
    TYPE_INDEX slotIdx;
    memcpy(&slotIdx,&block_buffer_in[sizeof(TYPE_DATA)*DATA_CHUNKS],sizeof(TYPE_INDEX));
    
	cout<< "	[recvBlock] Block Data RECV in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count() <<endl;
    server_logs[4] = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
    
	start = time_now;
    // Update root bucket
    FILE *file_update;
    string path = rootPath + to_string(this->serverNo) + "/0";
    if((file_update = fopen(path.c_str(),"r+b")) == NULL)
    {
        cout<< "	[recvBlock] File Cannot be Opened!!" <<endl;
        exit(0);
    }
    fseek(file_update, slotIdx*sizeof(TYPE_DATA),SEEK_SET);
    for(int u = 0 ; u < DATA_CHUNKS; u++)
    {
        fwrite(&block_buffer_in[u*sizeof(TYPE_DATA)],1,sizeof(TYPE_DATA),file_update);
        fseek(file_update,(BUCKET_SIZE-1)*sizeof(TYPE_DATA),SEEK_CUR);
    }
    fclose(file_update);
    
    end = time_now;
	cout<< "	[recvBlock] Block STORED in Disk in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count() <<endl;
	server_logs[5] = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
	
    socket.send((unsigned char*)CMD_SUCCESS,sizeof(CMD_SUCCESS));
	cout<< "	[recvBlock] ACK is SENT!" <<endl;
    
    return 0;
}




/**
 * Function Name: thread_socket_func & send & recv
 *
 * Description: Generic threaded socket functions for send and receive operations
 * 
 * @return 0 if successful
 */  
void *ServerS3ORAM::thread_socket_func(void* args)
{
    struct_socket* opt = (struct_socket*) args;
	
	if(opt->isSend)
	{
		auto start = time_now;
		send(opt->peer_idx, opt->data_out, opt->data_out_size);
		auto end = time_now;
		if(thread_max < std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count())
			thread_max = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
	}
	else
	{
		recv(opt->peer_idx, opt->data_in, opt->data_in_size);
	}
    pthread_exit((void*)opt);
}
int ServerS3ORAM::send(int peer_idx, unsigned char* input, size_t inputSize)
{
    unsigned char buffer_in[sizeof(CMD_SUCCESS)];
	
    try
    {
		socket_send[peer_idx]->send (input, inputSize);
		cout<< "	[ThreadedSocket] Data SENT!" << endl;
        
        socket_send[peer_idx]->recv(buffer_in, sizeof(CMD_SUCCESS));
        cout<< "	[ThreadedSocket] ACK RECEIVED!" << endl;
    }
    catch (exception &ex)
    {
        exit(0);
    }

	return 0;
}
int ServerS3ORAM::recv(int peer_idx, unsigned char* output, size_t outputSize)
{
    try
    {
		socket_recv[peer_idx]->recv (output, outputSize);
		cout<< "	[ThreadedSocket] Data RECEIVED! " <<endl;
        
        socket_recv[peer_idx]->send((unsigned char*)CMD_SUCCESS,sizeof(CMD_SUCCESS));
        cout<< "	[ThreadedSocket] ACK SENT! " <<endl;
    }
    catch (exception &ex)
    {
        cout<<"Socket error!";
        exit(0);
    }
    
	return 0;
}


/**
 * Function Name: thread_dotProduct_func //inherent
 *
 * Description: Threaded dot-product operation 
 * 
 */  
void *ServerS3ORAM::thread_dotProduct_func(void* args)
{
    THREAD_COMPUTATION* opt = (THREAD_COMPUTATION*) args;
  for(int k = opt->startIdx; k < opt->endIdx; k++)
    {
        opt->dot_product_output[k] = InnerProd_LL(opt->data_vector[k],opt->select_vector,opt->vector_length,P,zz_p::ll_red_struct());
    }
}


/**
 * Function Name: thread_crossProduct_func //inherent
 *
 * Description: Threaded cross-product operation 
 * 
 */  
void *ServerS3ORAM::thread_crossProduct_func(void* args)
{
    THREAD_COMPUTATION* opt = (THREAD_COMPUTATION*) args;
    
    
    for(int l = opt->startIdx ; l < opt->endIdx; l++) //fix this later
    {
        for(int k = 0 ; k < opt->output_length; k++)
        {
            opt->cross_product_output[l][k] = InnerProd_LL(opt->data_vector_triplet[l],opt->evict_matrix[k],opt->vector_length,P,zz_p::ll_red_struct());
        }
    }
    
    /*
    int currBucket;
	int currIndex;
	//TYPE_INDEX n = BUCKET_SIZE;
    //std::cout << " CPU # " << sched_getcpu() << "\n";
	for(int l = opt->startIdx ; l < opt->endIdx; l++)
    {
        currBucket = l/BUCKET_SIZE; //this can be removed later
		currIndex = l % BUCKET_SIZE; 
		
        for(int k = 0 ; k < DATA_CHUNKS; k++)
        {
            opt->cross_product_output[k][currBucket*BUCKET_SIZE + currIndex] = InnerProd_LL(opt->data_vector_triplet[k],opt->evict_matrix[l],opt->vector_length,P,zz_p::ll_red_struct());
        }
    }*/
    pthread_exit((void*)opt);
}


/**
 * Function Name: thread_loadRetrievalData_func
 *
 * Description: Threaded load function to read buckets in a path from disk storage
 * 
 */  
void* ServerS3ORAM::thread_loadRetrievalData_func(void* args)
{
    THREAD_LOADDATA* opt = (THREAD_LOADDATA*) args;
    
    unsigned long int load_time = 0;
    FILE* file_in = NULL;
    string path;
    
    for(int i = 0; i < opt->fullPathIdx_length; i++)
    {
        file_in = NULL;
        path = rootPath + to_string(opt->serverNo) + "/" + to_string(opt->fullPathIdx[i]);
        if((file_in = fopen(path.c_str(),"rb")) == NULL){
            cout<< "	[SendBlock] File cannot be opened!!" <<endl;
            exit;
        }
        fseek(file_in,BUCKET_SIZE*(opt->startIdx)*sizeof(TYPE_DATA),SEEK_SET);
        for (int k = opt->startIdx ; k < opt->endIdx; k++)
        {
            for(int j = 0 ; j < BUCKET_SIZE; j ++)
            {
                fread(&opt->data_vector[k][i*BUCKET_SIZE+j],1,sizeof(TYPE_DATA),file_in);
            }
        }
        fclose(file_in);
    }
}


/**
 * Function Name: thread_loadBucket_func
 *
 * Description: Threaded load function to read triplet buckets from disk storage
 * 
 */  
void* ServerS3ORAM::thread_loadBucket_func(void* args)
{
    THREAD_LOADDATA* opt = (THREAD_LOADDATA*) args;
    
    FILE* file_in = NULL;
    string path;

    file_in = NULL;
    
    TYPE_ID BucketIdx = opt->idx;
    unsigned long long currBufferIdx = 0;
    int n = 1;
    if(BucketIdx >= (NUM_NODES-2*N_leaf)) //at leaf level, load leaf bucket & auxiliary bucket
    {
        n=2;
    }
    for(int i = 0 ; i < n ; i ++)
    {
        path = rootPath + to_string(opt->serverNo) + "/" + to_string(BucketIdx);
        
        if((file_in = fopen(path.c_str(),"rb")) == NULL)
        {
            cout<< "	[SendBlock] File Cannot be Opened!!" <<endl;
            exit(0);
        }
        fseek(file_in,BUCKET_SIZE*(opt->startIdx)*sizeof(TYPE_DATA),SEEK_SET);
            
        for (int k = opt->startIdx ; k < opt->endIdx; k++)
        {
            fread(&opt->data_vector[k][currBufferIdx],1,sizeof(TYPE_DATA)*BUCKET_SIZE,file_in);
        }
        fclose(file_in);
        BucketIdx += N_leaf;
        currBufferIdx += BUCKET_SIZE;
    }        
    
}



/**
 * Function Name: sendORAMTree
 *
 * Description: Distributes generated and shared ORAM buckets to servers over network
 * 
 * @return 0 if successful
 */  
 
int ServerS3ORAM::recvORAMTree(zmq::socket_t& socket)
{
    int ret = 1;
    for(int i = 0 ; i < NUM_NODES;i++)
    {
        socket.recv(bucket_buffer,BUCKET_SIZE*sizeof(TYPE_DATA)*DATA_CHUNKS,0);
        string path = rootPath + to_string(serverNo) + "/" + to_string(i);
    
        FILE* file_out = NULL;
        if((file_out = fopen(path.c_str(),"wb+")) == NULL)
        {
            cout<< "	[recvORAMTree] File Cannot be Opened!!" <<endl;
            exit(0);
        }
        fwrite(bucket_buffer, 1, BUCKET_SIZE*sizeof(TYPE_DATA)*DATA_CHUNKS, file_out);
        fclose(file_out);
        socket.send((unsigned char*)CMD_SUCCESS,sizeof(CMD_SUCCESS),0);
       
    }
	 cout<< "	[recvORAMTree] ACK is SENT!" <<endl;
	
	ret = 0;
    return ret ;
}