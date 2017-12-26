/*
 * ServerS3ORAM.cpp
 *
 *  Created on: Apr 7, 2017
 *      Author: ceyhunozkaptan, thanghoang
 */

#include "ServerS3ORAM.hpp"
#include "Utils.hpp"
#include "struct_socket.h"

#include "S3ORAM.hpp"
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>

#include "struct_thread_computation.h"
#include "struct_thread_loadData.h"


unsigned long int ServerS3ORAM::server_logs[13];
unsigned long int ServerS3ORAM::thread_max = 0;
char ServerS3ORAM::timestamp[16];

ServerS3ORAM::ServerS3ORAM(TYPE_INDEX serverNo, int selectedThreads) 
{
	
//	this->CLIENT_ADDR = "tcp://*:" + SERVER_PORT[(serverNo)*NUM_SERVERS+serverNo];
	this->CLIENT_ADDR = "tcp://*:" + std::to_string(SERVER_PORT+(serverNo)*NUM_SERVERS+serverNo);
    
    this->numThreads = selectedThreads;
    this->thread_compute = new pthread_t[numThreads];
	
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
		
}

ServerS3ORAM::ServerS3ORAM()
{
}

ServerS3ORAM::~ServerS3ORAM()
{
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
			case CMD_SEND_ORAM_TREE:
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
			case CMD_REQUEST_BLOCK:
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
            case CMD_SEND_BLOCK:
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
			case CMD_SEND_EVICT:
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


/**
 * Function Name: retrieve
 *
 * Description: Starts retrieve operation for a block by receiving logical access vector and path ID from the client. 
 * According to path ID, server performs dot-product operation between its block shares on the path and logical access vector.
 * The result of the dot-product is send back to the client.
 * 
 * @param socket: (input) ZeroMQ socket instance for communication with the client
 * @return 0 if successful
 */  
int ServerS3ORAM::retrieve(zmq::socket_t& socket)
{
	Utils::write_list_to_file(to_string(HEIGHT) + "_" + to_string(BLOCK_SIZE) + "_server" + to_string(serverNo)+ "_" + timestamp + ".txt",logDir, server_logs, 13);
	memset(server_logs, 0, sizeof(unsigned long int)*13);
	
	int ret = 1;
	
	auto start = time_now;
	socket.recv(select_buffer_in,sizeof(TYPE_INDEX)+(H+1)*BUCKET_SIZE*sizeof(TYPE_DATA),0);
	auto end = time_now;
	cout<< "	[SendBlock] PathID and Logical Vector RECEIVED in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count() << " ns" <<endl;
    server_logs[0] = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
	
	TYPE_INDEX pathID;
	memcpy(&pathID, select_buffer_in, sizeof(pathID));
    
    
    zz_p sharedVector[(H+1)*BUCKET_SIZE];
    memcpy(sharedVector, &select_buffer_in[sizeof(pathID)], (H+1)*BUCKET_SIZE*sizeof(TYPE_DATA));
    cout<< "	[SendBlock] PathID is " << pathID <<endl;
	
    
    S3ORAM ORAM;
	TYPE_INDEX fullPathIdx[H+1];
    ORAM.getFullPathIdx(fullPathIdx, pathID);
	
    //use thread to load data from files
    start = time_now;
    int step = ceil((double)DATA_CHUNKS/(double)numThreads);
    int endIdx;
    THREAD_LOADDATA loadData_args[numThreads];
    for(int i = 0, startIdx = 0; i < numThreads , startIdx < DATA_CHUNKS; i ++, startIdx+=step)
    {
        if(startIdx+step > DATA_CHUNKS)
            endIdx = DATA_CHUNKS;
        else
            endIdx = startIdx+step;
            
        loadData_args[i] = THREAD_LOADDATA(this->serverNo, startIdx, endIdx, this->dot_product_vector, fullPathIdx,H+1);
        pthread_create(&thread_compute[i], NULL, &ServerS3ORAM::thread_loadRetrievalData_func, (void*)&loadData_args[i]);
        
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        pthread_setaffinity_np(thread_compute[i], sizeof(cpu_set_t), &cpuset);
    }
    
    for(int i = 0, startIdx = 0 ; i < numThreads , startIdx < DATA_CHUNKS; i ++, startIdx+=step)
    {
        pthread_join(thread_compute[i],NULL);
    }
    end = time_now;
	long load_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
    cout<< "	[SendBlock] Path Nodes READ from Disk in " << load_time << " ns"<<endl;
    server_logs[1] = load_time;

    start = time_now;
    //Multithread for dot product computation
    THREAD_COMPUTATION dotProduct_args[numThreads];
    endIdx = 0;
    step = ceil((double)DATA_CHUNKS/(double)numThreads);
    for(int i = 0, startIdx = 0 ; i < numThreads , startIdx < DATA_CHUNKS; i ++, startIdx+=step)
    {
        if(startIdx+step > DATA_CHUNKS)
            endIdx = DATA_CHUNKS;
        else
            endIdx = startIdx+step;
			
        dotProduct_args[i] = THREAD_COMPUTATION( startIdx, endIdx, this->dot_product_vector, sharedVector, sumBlock);
        pthread_create(&thread_compute[i], NULL, &ServerS3ORAM::thread_dotProduct_func, (void*)&dotProduct_args[i]);
		
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        pthread_setaffinity_np(thread_compute[i], sizeof(cpu_set_t), &cpuset);
    }
    
    for(int i = 0, startIdx = 0 ; i < numThreads , startIdx < DATA_CHUNKS; i ++, startIdx+=step)
    {
        pthread_join(thread_compute[i],NULL);
    }
    
    end = time_now;
    cout<< "	[SendBlock] Block Share CALCULATED in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count() <<endl;
    server_logs[2] = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();

    memcpy(block_buffer_out,sumBlock,sizeof(TYPE_DATA)*DATA_CHUNKS);
    
    start = time_now;
    cout<< "	[SendBlock] Sending Block Share with ID-" << sumBlock[0] <<endl;
    socket.send(block_buffer_out,sizeof(TYPE_DATA)*DATA_CHUNKS);
    end = time_now;
    cout<< "	[SendBlock] Block Share SENT in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count() <<endl;
    server_logs[3] = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
    
    ret = 0;
    return ret ;
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
 * Function Name: evict
 *
 * Description: Starts eviction operation with the command of the client by receiving eviction matrix
 * and eviction path no from the client. According to eviction path no, the server performs 
 * matrix multiplication with its buckets and eviction matrix to evict blocks. After eviction operation,
 * the degree of the sharing polynomial doubles. Thus all the servers distributes their shares and perform 
 * degree reduction routine simultaneously. 
 * 
 * @param socket: (input) ZeroMQ socket instance for communication with the client
 * @return 0 if successful
 */  
int ServerS3ORAM::evict(zmq::socket_t& socket)
{
    S3ORAM ORAM;
    TYPE_INDEX n_evict;
    
    int ret;
	
	cout<< "	[evict] Receiving Evict Matrix..." <<endl;;
	auto start = time_now;
	socket.recv(evict_buffer_in, (H+1)*evictMatSize*sizeof(TYPE_DATA) + sizeof(TYPE_INDEX), 0);
	auto end = time_now;
	cout<< "	[evict] RECEIVED! in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count() <<endl;
	server_logs[6] = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
	
	TYPE_INDEX evictPath;

	for (TYPE_INDEX y = 0 ; y < H+1 ; y++)
	{
		for (TYPE_INDEX i = 0 ; i < BUCKET_SIZE ; i++)
		{
			memcpy(this->evictMatrix[y][i], &evict_buffer_in[y*evictMatSize*sizeof(TYPE_DATA) + i*2*BUCKET_SIZE*sizeof(TYPE_DATA)], 2*BUCKET_SIZE*sizeof(TYPE_DATA));
		}
	}
    memcpy(&n_evict, &evict_buffer_in[(H+1)*evictMatSize*sizeof(TYPE_DATA)], sizeof(TYPE_INDEX));
	
    TYPE_INDEX srcIdx[H];
    TYPE_INDEX destIdx[H];
    TYPE_INDEX siblIdx[H];
    
    string evict_str = ORAM.getEvictString(n_evict);
    ORAM.getEvictIdx(srcIdx,destIdx,siblIdx,evict_str);
    
    for(int h = 0; h < H+1 ; h++)
    {
        cout<<endl;
		cout << "	==============================================================" << endl;
		cout<< "	[evict] Starting TripletEviction-" << h+1 <<endl;
		TYPE_INDEX curSrcIdx = srcIdx[h];
        TYPE_INDEX curDestIdx = destIdx[h];
        if(h == H) //for src-to-sibling bucket operation at leaf level
        {
            curSrcIdx = srcIdx[H-1];
            curDestIdx = siblIdx[H-1];
        }
        // Multithread for loading data from disk
		start = time_now;
		int step = ceil((double)DATA_CHUNKS/(double)numThreads);
		int endIdx;
		THREAD_LOADDATA loadData_args[numThreads];
		for(int i = 0, startIdx = 0; i < numThreads , startIdx < DATA_CHUNKS; i ++, startIdx+=step)
		{
			if(startIdx+step > DATA_CHUNKS)
				endIdx = DATA_CHUNKS;
			else
				endIdx = startIdx+step;
            
            loadData_args[i] = THREAD_LOADDATA(this->serverNo, startIdx, endIdx, this->cross_product_vector, curSrcIdx, curDestIdx);
			pthread_create(&thread_compute[i], NULL, &ServerS3ORAM::thread_loadTripletData_func, (void*)&loadData_args[i]);
			
			cpu_set_t cpuset;
			CPU_ZERO(&cpuset);
			CPU_SET(i, &cpuset);
			pthread_setaffinity_np(thread_compute[i], sizeof(cpu_set_t), &cpuset);
		}
		
		for(int i = 0, startIdx = 0 ; i < numThreads , startIdx < DATA_CHUNKS; i ++, startIdx+=step)
		{
			pthread_join(thread_compute[i],NULL);
		}
		end = time_now;
		long load_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
		cout<< "	[evict] Evict Nodes READ from Disk in " << load_time <<endl;
		server_logs[7] += load_time;
        
        //perform matrix product
        cout<< "	[evict] Multiplying Evict Matrix..." << endl;
		start = time_now;
		this->multEvictTriplet(this->evictMatrix[h]); 	// SERVER SIDE COMPUTATION
		end = time_now;
		cout<< "	[evict] Multiplied in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count() <<endl;
		server_logs[8] += std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();


		//== THREADS FOR LISTENING =======================================================================================
        struct_socket recvSocket_args[NUM_SERVERS-1];
		cout<< "	[evict] Creating Threads for Receiving Ports..." << endl;
		for(TYPE_INDEX k = 0; k < NUM_SERVERS-1; k++)
		{
//            recvSocket_args[k] = struct_socket("tcp://*:" + SERVER_PORT[(serverNo)*(NUM_SERVERS)+this->others[k]], NULL, 0, shares_buffer_in[k], BUCKET_SIZE*sizeof(TYPE_DATA)*DATA_CHUNKS, NULL,false);
			recvSocket_args[k] = struct_socket("tcp://*:" + std::to_string(SERVER_PORT+(serverNo)*(NUM_SERVERS)+this->others[k]), NULL, 0, shares_buffer_in[k], BUCKET_SIZE*sizeof(TYPE_DATA)*DATA_CHUNKS, NULL,false);
			pthread_create(&thread_recv[k], NULL, &ServerS3ORAM::thread_socket_func, (void*)&recvSocket_args[k]);
		}
		cout << "	[evict] CREATED!" << endl;
		//===============================================================================================================
		
		
		// Distribution & Degree Reduction
		TYPE_DATA* shares = new TYPE_DATA[NUM_SERVERS];
		
		cout<< "	[evict] Creating Shares for Reduction..." << endl;
		//boost::progress_display show_progress2((2*H+1)*BUCKET_SIZE);
		int m = 0;
        
		start = time_now;
		TYPE_INDEX curBuffIdx;
		for(int u = 0 ; u <DATA_CHUNKS; u++)
		{
            for(TYPE_INDEX j = 0; j < BUCKET_SIZE; j++)
            {
                ORAM.createShares(this->BUCKET_DATA[u][ j ], shares); // EACH SERVER CALCULATES AND DISTRIBUTES SHARES
                curBuffIdx = (u*BUCKET_SIZE) +  j; //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                for(TYPE_INDEX k = 0; k < NUM_SERVERS; k++)
                {
                    if (k == this->serverNo)
                    {
                        ownShares[this->serverNo][u][j] = shares[k];
                    }
                    else
                    {
                        memcpy(&shares_buffer_out[m][curBuffIdx*sizeof(TYPE_DATA)], &shares[k], sizeof(TYPE_DATA));
                        m++;
                    }
                }
                m = 0;
            //++show_progress2;
            }
        }
	   
		end = time_now;
		cout<< "	[evict] Shares CREATED in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count() <<endl;
		server_logs[9] += std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
		 
		delete shares;
		
		//== THREADS FOR SENDING ============================================================================================
		struct_socket sendSocket_args[NUM_SERVERS-1];
		cout<< "	[evict] Creating Threads for Sending Shares..."<< endl;;
		for (int i = 0; i < NUM_SERVERS-1; i++)
		{
			sendSocket_args[i] = struct_socket(SERVER_ADDR[this->others[i]] + ":" + std::to_string(SERVER_PORT+this->others[i]*NUM_SERVERS+this->serverNo),  shares_buffer_out[i], BUCKET_SIZE*sizeof(TYPE_DATA)*DATA_CHUNKS, NULL, 0, NULL, true);
//			sendSocket_args[i] = struct_socket(SERVER_ADDR[this->others[i]] + ":" + SERVER_PORT[this->others[i]*NUM_SERVERS+this->serverNo],  shares_buffer_out[i], BUCKET_SIZE*sizeof(TYPE_DATA)*DATA_CHUNKS, NULL, 0, NULL, true);
			pthread_create(&thread_send[i], NULL, &ServerS3ORAM::thread_socket_func, (void*)&sendSocket_args[i]);
		}
		cout<< "	[evict] CREATED!" <<endl;
		//=================================================================================================================
		cout<< "	[evict] Waiting for Threads..." <<endl;
		for (int i = 0; i < NUM_SERVERS-1; i++)
		{
			pthread_join(thread_send[i], NULL);
			pthread_join(thread_recv[i], NULL);
		}
		
		cout<< "	[evict] DONE!" <<endl;
		server_logs[10] += thread_max;
		thread_max = 0;
		
		cout << "	[evict] Writing Received Shares" << endl;
		for(int u = 0 ; u < DATA_CHUNKS; u ++)
		{
			m = 0;
			for(TYPE_INDEX k = 0; k < NUM_SERVERS; k++)
			{
				if (k == this->serverNo)
				{
					
				}
				else
				{
					memcpy(ownShares[k][u], &shares_buffer_in[m][u*BUCKET_SIZE*sizeof(TYPE_DATA)], BUCKET_SIZE*sizeof(TYPE_DATA));   
                    m++;
				}
			}
		}
		cout << "	[evict] WRITTEN!" << endl;
		
        memset(bucket_buffer,0,BUCKET_SIZE*sizeof(TYPE_DATA)*DATA_CHUNKS);

        cout << "	[evict] Calculating New Shares (Degree Reduction)" << endl;
        TYPE_DATA sum;
        
        start = time_now;
        for(int u = 0 ; u <DATA_CHUNKS; u++)
        {
            for(TYPE_INDEX j = 0; j < BUCKET_SIZE; j++)
            {
                sum = 0;
                for (TYPE_INDEX l = 0; l < NUM_SERVERS; l++)
                {
                    sum = (sum + Utils::mulmod(vandermonde[l], ownShares[l][u][j])) % P;
                }

                memcpy(&bucket_buffer[(u*BUCKET_SIZE+ j)*sizeof(TYPE_DATA)], &sum, sizeof(TYPE_DATA));
            }
        }
        end = time_now;
        server_logs[11] += std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
        
        //write to file
        start = time_now;
        FILE* file_out = NULL;
        string path;
        
        //overwrite (non-leaf) sibling bucket with source bucket
        if(h < H-1)
        {
            string path_src = rootPath + to_string(serverNo) + "/" + to_string(curSrcIdx);
            string path_sibl = rootPath + to_string(serverNo) + "/" + to_string(siblIdx[h]);
            std::ifstream src(path_src,std::ios::binary);
            std::ofstream sib(path_sibl,std::ios::binary);
            sib << src.rdbuf();
        }
        path = rootPath + to_string(serverNo) + "/" + to_string(curDestIdx); 
        
        if((file_out = fopen(path.c_str(),"wb+")) == NULL)
        {
            cout<< "	[evict] File Cannot be Opened!!" <<endl;
            exit(0);
        }
        fwrite(bucket_buffer, 1, BUCKET_SIZE*sizeof(TYPE_DATA)*DATA_CHUNKS, file_out);
        fclose(file_out);
        end = time_now;
        server_logs[12] += std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
        
        cout<< "	[evict] Reduction DONE in " << server_logs[11] <<endl;
        cout<< "	[evict] Written to Disk in " << server_logs[12] <<endl;
		cout<< "	[evict] TripletEviction-" << h+1 << " COMPLETED!"<<endl;
    }
    
    socket.send((unsigned char*)CMD_SUCCESS,sizeof(CMD_SUCCESS));
	cout<< "	[evict] ACK is SENT!" <<endl;

    return 0;
}


/**
 * Function Name: multEvictTriplet
 *
 * Description: Performs matrix multiplication between received eviction matrix and affected buckets
 * for eviction operation 
 * 
 * @param evictMatrix: (input) Received eviction matrix from the clietn
 * @return 0 if successful
 */  
int ServerS3ORAM::multEvictTriplet(zz_p** evictMatrix)
{
	TYPE_INDEX n = BUCKET_SIZE;
	
    //thread implementation
    THREAD_COMPUTATION crossProduct_args[numThreads];
    int endIdx;
    int step = ceil((double)n/(double)numThreads);
    for(int i = 0, startIdx = 0 ; i < numThreads; i ++, startIdx+=step)
    {
        if(startIdx+step > n)
            endIdx = n;
        else
            endIdx = startIdx+step;
    
        crossProduct_args[i] = THREAD_COMPUTATION(startIdx, endIdx, this->cross_product_vector,  evictMatrix, this->BUCKET_DATA );
        pthread_create(&thread_compute[i], NULL, &ServerS3ORAM::thread_crossProduct_func, (void*)&crossProduct_args[i]);
        
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        pthread_setaffinity_np(thread_compute[i], sizeof(cpu_set_t), &cpuset);
    }
    
    for(int i  = 0 ; i <numThreads ; i++)
    {
        pthread_join(thread_compute[i],NULL);
    }
	
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
		send(opt->ADDR, opt->data_out, opt->data_out_size);
		auto end = time_now;
		if(thread_max < std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count())
			thread_max = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
	}
	else
	{
		recv(opt->ADDR, opt->data_in, opt->data_in_size);
	}
    pthread_exit((void*)opt);
}
int ServerS3ORAM::send(std::string ADDR, unsigned char* input, size_t inputSize)
{
	zmq::context_t context(1);
    zmq::socket_t socket(context,ZMQ_REQ);

    socket.connect(ADDR.c_str());
	
    unsigned char buffer_in[sizeof(CMD_SUCCESS)];
	
    try
    {
		cout<< "	[ThreadedSocket] Sending to " << ADDR << endl;
		socket.send (input, inputSize);
		cout<< "	[ThreadedSocket] Data SENT!" << ADDR << endl;
        
        socket.recv(buffer_in, sizeof(CMD_SUCCESS));
        cout<< "	[ThreadedSocket] ACK RECEIVED!" << ADDR << endl;
    }
    catch (exception &ex)
    {
        goto exit;
    }

exit:
	socket.disconnect(ADDR.c_str());
	socket.close();
	return 0;
}
int ServerS3ORAM::recv(std::string ADDR, unsigned char* output, size_t outputSize)
{
	zmq::context_t context(1);
    zmq::socket_t socket(context,ZMQ_REP);
	
    socket.bind(ADDR.c_str());
	
    try
    {
		cout<< "	[ThreadedSocket] Waiting Client on " << ADDR << endl;
		socket.recv (output, outputSize);
		cout<< "	[ThreadedSocket] Data RECEIVED! " << ADDR <<endl;
        
        socket.send((unsigned char*)CMD_SUCCESS,sizeof(CMD_SUCCESS));
        cout<< "	[ThreadedSocket] ACK SENT! "  << ADDR<<endl;
    }
    catch (exception &ex)
    {
        cout<<"Socket error!";
        goto exit;
    }
    
exit:
	socket.close();
	return 0;
}


/**
 * Function Name: thread_dotProduct_func
 *
 * Description: Threaded dot-product operation 
 * 
 */  
void *ServerS3ORAM::thread_dotProduct_func(void* args)
{
    THREAD_COMPUTATION* opt = (THREAD_COMPUTATION*) args;
  
    //std::cout << " CPU # " << sched_getcpu() << "\n";
    int size = (H+1)*BUCKET_SIZE;
    for(int k = opt->startIdx; k < opt->endIdx; k++)
    {
        opt->dot_product_output[k] = InnerProd_LL(opt->data_vector[k],opt->select_vector,size,P,zz_p::ll_red_struct());
    }
}


/**
 * Function Name: thread_crossProduct_func
 *
 * Description: Threaded cross-product operation 
 * 
 */  
void *ServerS3ORAM::thread_crossProduct_func(void* args)
{
    THREAD_COMPUTATION* opt = (THREAD_COMPUTATION*) args;
    
    int currBucket;
	int currIndex;
	TYPE_INDEX n = BUCKET_SIZE*(2);
    //std::cout << " CPU # " << sched_getcpu() << "\n";
	for(int l = opt->startIdx ; l < opt->endIdx; l++)
    {
        currBucket = l/BUCKET_SIZE; //this can be removed later
		currIndex = l % BUCKET_SIZE; 
		
        for(int k = 0 ; k < DATA_CHUNKS; k++)
        {
            opt->cross_product_output[k][currBucket*BUCKET_SIZE + currIndex] = InnerProd_LL(opt->data_vector_triplet[k],opt->evict_matrix[l],n,P,zz_p::ll_red_struct());
        }
    }
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
 * Function Name: thread_loadTripletData_func
 *
 * Description: Threaded load function to read triplet buckets from disk storage
 * 
 */  
void* ServerS3ORAM::thread_loadTripletData_func(void* args)
{
    THREAD_LOADDATA* opt = (THREAD_LOADDATA*) args;
    
    unsigned long int load_time = 0;
    FILE* file_in = NULL;
    string path;

    for(int i = 0; i < 2; i++)
    {
        file_in = NULL;
        path = rootPath + to_string(opt->serverNo) + "/" + to_string(opt->idx[i]);
        if((file_in = fopen(path.c_str(),"rb")) == NULL)
        {
            cout<< "	[SendBlock] File Cannot be Opened!!" <<endl;
            exit(0);
        }
        fseek(file_in,BUCKET_SIZE*(opt->startIdx)*sizeof(TYPE_INDEX),SEEK_SET);
		
        for (int k = opt->startIdx ; k < opt->endIdx; k++)
        {
            fread(&opt->data_vector[k][i*BUCKET_SIZE],1,sizeof(TYPE_DATA)*BUCKET_SIZE,file_in);
        }
        fclose(file_in);
    }
}




