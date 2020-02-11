/*
 * ServerKaryS3ORAMO.cpp
 *
 *  Created on: Apr 7, 2017
 *      Author: ceyhunozkaptan, thanghoang
 */

#include "ServerKaryS3ORAMO.hpp"

#include "Utils.hpp"
#include "S3ORAM.hpp"
#include "struct_socket.h"
#include "struct_thread_computation.h"
#include "struct_thread_loadData.h"

ServerKaryS3ORAMO::ServerKaryS3ORAMO(TYPE_INDEX serverNo, int selectedThreads) : ServerBinaryS3ORAMO(serverNo, selectedThreads)
{
    //specific
    for(TYPE_INDEX y = 0 ; y < H; y++)
	{
		for(TYPE_INDEX i = 0 ; i < BUCKET_SIZE; i++)
		{
			delete[] this->evictMatrix[y][i];
		}
        delete[] this->evictMatrix[y];
	}
    delete[] this->evictMatrix[H];
    this->evictMatrix[H] = new zz_p*[BUCKET_SIZE];
    for(TYPE_INDEX i = 0 ; i < BUCKET_SIZE; i++)
    {
        this->evictMatrix[H][i] = new zz_p[2*BUCKET_SIZE];
    }
    
    //H+2, instead of H +1 due to auxiliary bucket at leaf nodes
    delete[] this->select_buffer_in;
	for (TYPE_INDEX k = 0 ; k < DATA_CHUNKS; k++)
	{
        delete[] this->dot_product_vector[k];
	}
    delete[] this->evict_buffer_in;
	
	
    
    
	for(TYPE_INDEX y = 0 ; y < H; y++)
	{
        this->evictMatrix[y] = new zz_p*[BUCKET_SIZE];
		for(TYPE_INDEX i = 0 ; i < BUCKET_SIZE; i++)
		{
			this->evictMatrix[y][i] = new zz_p[BUCKET_SIZE];
		}
	}
    this->evictMatrix[H] = new zz_p*[BUCKET_SIZE];
    for(TYPE_INDEX i = 0 ; i < BUCKET_SIZE; i++)
    {
        this->evictMatrix[H][i] = new zz_p[2*BUCKET_SIZE];
    }
    
    //H+2, instead of H +1 due to auxiliary bucket at leaf nodes
    this->select_buffer_in = new unsigned char[sizeof(TYPE_INDEX)+(H+2)*BUCKET_SIZE*sizeof(TYPE_DATA)];
	for (TYPE_INDEX k = 0 ; k < DATA_CHUNKS; k++)
	{
		this->dot_product_vector[k] = new zz_p[BUCKET_SIZE*(H+2)];
	}
	this->evict_buffer_in = new unsigned char[(H+2)*evictMatSize*sizeof(TYPE_DATA) + sizeof(TYPE_INDEX)];
	
   
        
    this->cross_product_vector_aux = new zz_p*[DATA_CHUNKS];
    for (TYPE_INDEX k = 0 ; k < DATA_CHUNKS; k++)
	{
		this->cross_product_vector_aux[k]  = new zz_p[2*BUCKET_SIZE];	
	}
    
		
}

ServerKaryS3ORAMO::ServerKaryS3ORAMO()
{
}

ServerKaryS3ORAMO::~ServerKaryS3ORAMO()
{
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
int ServerKaryS3ORAMO::retrieve(zmq::socket_t& socket)
{
	Utils::write_list_to_file(to_string(HEIGHT) + "_" + to_string(BLOCK_SIZE) + "_server" + to_string(serverNo)+ "_" + timestamp + ".txt",logDir, server_logs, 13);
	memset(server_logs, 0, sizeof(unsigned long int)*13);
	
	int ret = 1;
	
	auto start = time_now;
	socket.recv(select_buffer_in,sizeof(TYPE_INDEX)+(H+2)*BUCKET_SIZE*sizeof(TYPE_DATA),0);
	auto end = time_now;
	cout<< "	[SendBlock] PathID and Logical Vector RECEIVED in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count() << " ns" <<endl;
    server_logs[0] = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
	
	TYPE_INDEX pathID;
	memcpy(&pathID, select_buffer_in, sizeof(pathID));
    
    
    zz_p sharedVector[(H+2)*BUCKET_SIZE];
    memcpy(sharedVector, &select_buffer_in[sizeof(pathID)], (H+2)*BUCKET_SIZE*sizeof(TYPE_DATA));
    cout<< "	[SendBlock] PathID is " << pathID <<endl;
	
    
    S3ORAM ORAM;
	TYPE_INDEX fullPathIdx[H+2];
    ORAM.getFullPathIdx(fullPathIdx, pathID);
	
    //auxiliary bucket at leaf level
    fullPathIdx[H+1] = fullPathIdx[H] + N_leaf;



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
            
        loadData_args[i] = THREAD_LOADDATA(this->serverNo, startIdx, endIdx, this->dot_product_vector, fullPathIdx,H+2);
        pthread_create(&thread_compute[i], NULL, &ServerS3ORAM::thread_loadRetrievalData_func, (void*)&loadData_args[i]);
        
        /*cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        pthread_setaffinity_np(thread_compute[i], sizeof(cpu_set_t), &cpuset);*/
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
			
        dotProduct_args[i] = THREAD_COMPUTATION( startIdx, endIdx, this->dot_product_vector, sharedVector, (H+2)*BUCKET_SIZE, sumBlock);
        pthread_create(&thread_compute[i], NULL, &ServerS3ORAM::thread_dotProduct_func, (void*)&dotProduct_args[i]);
		
        /*cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        pthread_setaffinity_np(thread_compute[i], sizeof(cpu_set_t), &cpuset);*/
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
int ServerKaryS3ORAMO::evict(zmq::socket_t& socket)
{
    S3ORAM ORAM;
    TYPE_INDEX n_evict;
    
    int ret;
	
	cout<< "	[evict] Receiving Evict Matrix..." <<endl;;
	auto start = time_now;
	socket.recv(evict_buffer_in, (H+2)*evictMatSize*sizeof(TYPE_DATA) + sizeof(TYPE_INDEX), 0);
	auto end = time_now;
	cout<< "	[evict] RECEIVED! in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count() <<endl;
	server_logs[6] = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
	
	TYPE_INDEX evictPath;

	for (TYPE_INDEX y = 0 ; y < H ; y++)
	{
		for (TYPE_INDEX i = 0 ; i < BUCKET_SIZE ; i++)
		{
			memcpy(this->evictMatrix[y][i], &evict_buffer_in[y*evictMatSize*sizeof(TYPE_DATA) + i*BUCKET_SIZE*sizeof(TYPE_DATA)], BUCKET_SIZE*sizeof(TYPE_DATA));
		}
	}
    for (TYPE_INDEX i = 0 ; i < BUCKET_SIZE ; i++)
    {
        memcpy(this->evictMatrix[H][i], &evict_buffer_in[H*evictMatSize*sizeof(TYPE_DATA) + i*2*BUCKET_SIZE*sizeof(TYPE_DATA)], 2*BUCKET_SIZE*sizeof(TYPE_DATA));
    }
	
    memcpy(&n_evict, &evict_buffer_in[(H+2)*evictMatSize*sizeof(TYPE_DATA)], sizeof(TYPE_INDEX));
	
    
    string strEvict = ORAM.getEvictString(n_evict);
    TYPE_INDEX fullEvictPathIdx[H+1];
    ORAM.getFullEvictPathIdx(fullEvictPathIdx,strEvict);
    
    FILE* file_out[K_ARY];
    string path;
            
    
    for(int h = 0; h < H+1 ; h++)
    {
        cout<<endl;
		cout << "	==============================================================" << endl;
		cout<< "	[evict] Starting Eviction-" << h+1 <<endl;
		
        TYPE_INDEX currBucketIdx = fullEvictPathIdx[h];
        
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
            if(h<H)
            {
                loadData_args[i] = THREAD_LOADDATA(this->serverNo, startIdx, endIdx, this->cross_product_vector, currBucketIdx);
			}
            else
            {
                loadData_args[i] = THREAD_LOADDATA(this->serverNo, startIdx, endIdx, this->cross_product_vector_aux, currBucketIdx);
            }
            pthread_create(&thread_compute[i], NULL, &ServerS3ORAM::thread_loadBucket_func, (void*)&loadData_args[i]);
			
			/*cpu_set_t cpuset;
			CPU_ZERO(&cpuset);
			CPU_SET(i, &cpuset);
			pthread_setaffinity_np(thread_compute[i], sizeof(cpu_set_t), &cpuset);*/
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
		this->multEvictTriplet(h); 	// SERVER SIDE COMPUTATION
		end = time_now;
		cout<< "	[evict] Multiplied in " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count() <<endl;
		server_logs[8] += std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();


		//== THREADS FOR LISTENING =======================================================================================
        struct_socket recvSocket_args[NUM_SERVERS-1];
		cout<< "	[evict] Creating Threads for Receiving Ports..." << endl;
		for(TYPE_INDEX k = 0; k < NUM_SERVERS-1; k++)
		{
			recvSocket_args[k] = struct_socket(k, NULL, 0, shares_buffer_in[k], BUCKET_SIZE*sizeof(TYPE_DATA)*DATA_CHUNKS, NULL,false);
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
			sendSocket_args[i] = struct_socket(i,  shares_buffer_out[i], BUCKET_SIZE*sizeof(TYPE_DATA)*DATA_CHUNKS, NULL, 0, NULL, true);
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
        
        //write to the deeper Bucket
        
        start = time_now;
        
        if(h < H)
        {
            int slideIdx = (strEvict[h]-'0');
            for(int i = 0 ; i < K_ARY; i++)
            {
                TYPE_INDEX ChildBucketID = currBucketIdx*K_ARY + (i+1);
                path = rootPath + to_string(serverNo) + "/" + to_string(ChildBucketID); 
                if((file_out[i] = fopen(path.c_str(),"r+b")) == NULL)
                {
                    cout<< "	[evict] File Cannot be Opened!!" <<endl;
                    exit(0);
                }
                fseek(file_out[i], slideIdx*(BUCKET_SIZE*sizeof(TYPE_DATA)/K_ARY),SEEK_SET);
            }
            unsigned long long currBufferIdx = 0;
            for(int c = 0 ; c < DATA_CHUNKS; c++)
            {
                for(int i = 0 ; i < K_ARY; i++)
                {
                    fwrite(&bucket_buffer[currBufferIdx],1,sizeof(TYPE_DATA)*BUCKET_SIZE/K_ARY,file_out[i]);
                    fseek(file_out[i], (K_ARY-1)*(BUCKET_SIZE*sizeof(TYPE_DATA)/K_ARY),SEEK_CUR);
                    currBufferIdx+=sizeof(TYPE_DATA)*BUCKET_SIZE/K_ARY;
                }
                
            }
            for(int i = 0 ; i < K_ARY; i++)
                fclose(file_out[i]);
            
        }
        else //h == H
        {
            path = rootPath + to_string(serverNo) + "/" + to_string(currBucketIdx+N_leaf); 
            if((file_out[0] = fopen(path.c_str(),"wb+")) == NULL)
            {
                cout<< "	[evict] File Cannot be Opened!!" <<endl;
                exit(0);
            }
            fwrite(bucket_buffer,1,sizeof(TYPE_DATA)*BUCKET_SIZE*DATA_CHUNKS,file_out[0]);
            fclose(file_out[0]);
        }
            
        /*
        
        
        end = time_now;
        server_logs[12] += std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
        */
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
int ServerKaryS3ORAMO::multEvictTriplet(int level)
{
	
    //thread implementation
    THREAD_COMPUTATION crossProduct_args[numThreads];
    int endIdx;
    int step = ceil((double)DATA_CHUNKS/(double)numThreads);
    
    for(int i = 0, startIdx = 0 ; i < numThreads; i ++, startIdx+=step)
    {
        if(startIdx+step > DATA_CHUNKS)
            endIdx = DATA_CHUNKS;
        else
            endIdx = startIdx+step;
        if(level == H)
        {
            crossProduct_args[i] = THREAD_COMPUTATION(startIdx, endIdx, this->cross_product_vector_aux, BUCKET_SIZE*2, BUCKET_SIZE,  evictMatrix[H], this->BUCKET_DATA );
            pthread_create(&thread_compute[i], NULL, &ServerS3ORAM::thread_crossProduct_func, (void*)&crossProduct_args[i]);
        }
        else
        {
            crossProduct_args[i] = THREAD_COMPUTATION(startIdx, endIdx, this->cross_product_vector, BUCKET_SIZE, BUCKET_SIZE, evictMatrix[level], this->BUCKET_DATA );
            pthread_create(&thread_compute[i], NULL, &ServerS3ORAM::thread_crossProduct_func, (void*)&crossProduct_args[i]);
        }        
        /*cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        pthread_setaffinity_np(thread_compute[i], sizeof(cpu_set_t), &cpuset);*/
    }
    
    for(int i  = 0 ; i <numThreads ; i++)
    {
        pthread_join(thread_compute[i],NULL);
    }
	
	return 0;
}






