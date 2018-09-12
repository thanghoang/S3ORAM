/*
 * S3ORAM.cpp
 *
 *  Created on: Mar 15, 2017
 *      Author: ceyhunozkaptan, thanghoang
 */
 
#include "S3ORAM.hpp"
#include "Utils.hpp"


S3ORAM::S3ORAM()
{
}

S3ORAM::~S3ORAM()
{
}


/**
 * Function Name: build
 *
 * Description: Builds ORAM buckets with random data based on generated position map
 * and creates shares for distributed servers are created from ORAM buckets. 
 * Buckets are stored in the disk storage as seperate files.
 * 
 * @param pos_map: (input) Randomly generated position map to build ORAM buckets
 * @param metaData: (output) metaData of position map for scanning optimizations
 * @return 0 if successful
 */   
int S3ORAM::build(TYPE_POS_MAP* pos_map, TYPE_ID** metaData)
{
	int div = ceil(NUM_BLOCK/(double)N_leaf);
	assert(div <= BUCKET_SIZE && "ERROR: CHECK THE PARAMETERS => LEAVES CANNOT STORE ALL");
	
  	TYPE_DATA** bucket = new TYPE_DATA*[DATA_CHUNKS];
    for (int i = 0 ; i < DATA_CHUNKS; i++ )
    {
        bucket[i] = new TYPE_DATA[BUCKET_SIZE];
        memset(bucket[i],0,sizeof(TYPE_DATA)*BUCKET_SIZE);
    }
    
    TYPE_DATA** temp = new TYPE_DATA*[DATA_CHUNKS];
    for (int i = 0 ; i < DATA_CHUNKS; i++ )
    {
        temp[i] = new TYPE_DATA[BUCKET_SIZE];
        memset(temp[i],0,sizeof(TYPE_DATA)*BUCKET_SIZE);
    }
    FILE* file_out = NULL;
    string path;
		
    cout << "=================================================================" << endl;
    cout<< "[S3ORAM] Creating Buckets on Disk" << endl;
    
    boost::progress_display show_progress2(NUM_NODES);
    
    //generate bucket ID pools
    vector<TYPE_ID> blockIDs;
    for(TYPE_ID i = 0; i <NUM_BLOCK;i++)
    {
        blockIDs.push_back(i+1);
    }
    //random permutation using built-in function
    std::random_shuffle ( blockIDs.begin(), blockIDs.end() );
    
    
    //non-leaf buckets are all empty
    for(TYPE_INDEX i = 0 ; i < NUM_NODES/2; i ++)
    {
        file_out = NULL;
        path = clientDataDir + to_string(i);
        if((file_out = fopen(path.c_str(),"wb+")) == NULL)
        {
            cout<< "[S3ORAM] File Cannot be Opened!!" <<endl;
            exit(0);
        }
        for(int ii = 0 ; ii <DATA_CHUNKS; ii++)
        {
            fwrite(bucket[ii], 1, BUCKET_SIZE*sizeof(TYPE_DATA), file_out);
        }
        fclose(file_out);
    }
    
    //generate random blocks in leaf-buckets
    TYPE_INDEX iter= 0;
    for(TYPE_INDEX i = NUM_NODES/2 ; i < NUM_NODES; i++)
    {
        memset(bucket[0],0,sizeof(TYPE_DATA)*BUCKET_SIZE);
        for(int ii = BUCKET_SIZE/2 ; ii<BUCKET_SIZE; ii++)
        {
            if(iter>=NUM_BLOCK)
                break;
            bucket[0][ii] = blockIDs[iter];
            pos_map[blockIDs[iter]].pathID = i;
            pos_map[blockIDs[iter]].pathIdx = ii+(BUCKET_SIZE*H)  ;
            metaData[i][ii]= blockIDs[iter];
            
            iter++;
            
        }
        //write bucket to file
        file_out = NULL;
        path = clientDataDir + to_string(i);
        if((file_out = fopen(path.c_str(),"wb+")) == NULL)
        {
            cout<< "[S3ORAM] File Cannot be Opened!!" <<endl;
            exit(0);
        }
        for(int ii = 0 ; ii < DATA_CHUNKS; ii++)
        {
            fwrite(bucket[ii], 1, BUCKET_SIZE*sizeof(TYPE_DATA), file_out);
        }
        fclose(file_out);
    }
    
    
    cout << "=================================================================" << endl;
    cout<< "[S3ORAM] Creating Shares on Disk" << endl;
    boost::progress_display show_progress(NUM_NODES);
    
    TYPE_DATA*** bucketShares = new TYPE_DATA**[NUM_SERVERS];
    for(TYPE_INDEX k = 0; k < NUM_SERVERS; k++)
    {
        bucketShares[k] = new TYPE_DATA*[DATA_CHUNKS];
        for(int i = 0 ; i < DATA_CHUNKS ; i++ )
        {
            bucketShares[k][i] = new TYPE_DATA[BUCKET_SIZE];
        }
    }
        
		
    TYPE_DATA shares[DATA_CHUNKS][NUM_SERVERS];
    FILE* file_in = NULL;
    file_out = NULL;
        
    auto start = time_now;
    auto end = time_now;
    
    for (TYPE_INDEX i = 0; i < NUM_NODES; i++)
    {
        path = clientDataDir + to_string(i);
        if((file_in = fopen(path.c_str(),"rb")) == NULL){
            cout<< "[S3ORAM] File Cannot be Opened!!" <<endl;
            exit(0);
        }
        for(int ii = 0 ; ii < DATA_CHUNKS; ii++)
        {
        
            fread(bucket[ii] ,1 , BUCKET_SIZE*sizeof(TYPE_DATA), file_in);
            for(TYPE_INDEX j = 0; j < BUCKET_SIZE; j++)
            {           
                createShares(bucket[ii][j], shares[ii]);         
                for(TYPE_INDEX k = 0; k < NUM_SERVERS; k++)  
                {
                    memcpy(&bucketShares[k][ii][j], &shares[ii][k], sizeof(TYPE_DATA));
                }
            }
        }
        
        fclose(file_in);
        
        start = time_now;
        for(TYPE_INDEX k = 0; k < NUM_SERVERS; k++) 
        {
            path = rootPath + to_string(k) + "/" + to_string(i);
            if((file_out = fopen(path.c_str(),"wb+")) == NULL)
            {
                cout<< "[S3ORAM] File Cannot be Opened!!" <<endl;
                exit;
            }
            for(int ii = 0 ; ii< DATA_CHUNKS ; ii++)
            {
                fwrite(bucketShares[k][ii], 1, BUCKET_SIZE*sizeof(TYPE_DATA), file_out);
            }
            fclose(file_out);
        }    
        end = time_now;
        ++show_progress;
    }
		
    	
    cout<< "[S3ORAM] Elapsed Time for Init on Disk: "<< std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count() <<" ns"<<endl;
		
    for(TYPE_INDEX k = 0; k < NUM_SERVERS; k++)
    {
        for(int i = 0 ; i < DATA_CHUNKS; i++)
        {
            delete bucketShares[k][i];
        }
        delete bucketShares[k];
    }
    delete bucketShares;
    for(int i = 0 ; i < DATA_CHUNKS; i++)
    {
        delete bucket[i];
        delete temp[i];
    }
    delete bucket;
    delete temp;

	cout<<endl;
	cout << "=================================================================" << endl;
	cout<< "[S3ORAM] Shared ORAM Tree is Initialized for " << NUM_SERVERS << " Servers" <<endl;
	cout << "=================================================================" << endl;
	cout<<endl;
	
	return 0;
}


/**
 * Function Name: getEvictIdx
 *
 * Description: Determine the indices of source bucket, destination bucket and sibling bucket 
 * residing on the eviction path
 * 
 * @param srcIdx: (output) source bucket index array
 * @param destIdx: (output) destination bucket index array
 * @param siblIdx: (output) sibling bucket index array
 * @param str_evict: (input) eviction edges calculated by binary ReverseOrder of the eviction number 
 * @return 0 if successful
 */     
int S3ORAM::getEvictIdx (TYPE_INDEX *srcIdx, TYPE_INDEX *destIdx, TYPE_INDEX *siblIdx, string str_evict)
{
    srcIdx[0] = 0;
    if (str_evict[0]-'0' == 0) 
    {
        destIdx[0] = 1;
        siblIdx[0]  = 2;
    }
    else
    {
        destIdx[0] = 2;
        siblIdx[0] = 1;
    }
    for(int i = 0 ; i < H ; i ++)
    {
        if (str_evict[i]-'0' == 1 )
        {
            if(i < H-1)
                srcIdx[i+1] = srcIdx[i]*2 +2;
            if(i > 0)
            {
            
                destIdx[i] = destIdx[i-1]*2 +2;
                siblIdx[i] = destIdx[i-1]*2+1;
            }
        }
        else
        {
            if(i<H-1)
                srcIdx[i+1] = srcIdx[i]*2 + 1;
            if(i > 0 )
            {
                destIdx[i] = destIdx[i-1]*2 + 1;
                siblIdx[i] = destIdx[i-1]*2 + 2;
            }
        }
    }
    
	return 0;
}


/**
 * Function Name: getEvictString
 *
 * Description: Generates the path for eviction acc. to eviction number based on reverse 
 * lexicographical order. 
 * [For details refer to 'Optimizing ORAM and using it efficiently for secure computation']
 * 
 * @param n_evict: (input) The eviction number
 * @return Bit sequence of reverse lexicographical eviction order
 */  
string S3ORAM::getEvictString(TYPE_ID n_evict)
{
    string s = std::bitset<H>(n_evict).to_string();
    reverse(s.begin(),s.end());
    return s;
}


/**
 * Function Name: getFullPathIdx
 *
 * Description: Creates array of the indexes of the buckets that are on the given path
 * 
 * @param fullPath: (output) The array of the indexes of buckets that are on given path
 * @param pathID: (input) The leaf ID based on the index of the bucket in ORAM tree.
 * @return 0 if successful
 */  
int S3ORAM::getFullPathIdx(TYPE_INDEX* fullPath, TYPE_INDEX pathID)
{
    TYPE_INDEX idx = pathID;
    for (int i = H; i >= 0; i--)
    {
			fullPath[i] = idx;
			idx = (idx-1) >> 1;
    }
	
	return 0;
}


/**
 * Function Name: createShares
 *
 * Description: Creates shares from an input based on Shamir's Secret Sharing algorithm
 * 
 * @param input: (input) The secret to be shared
 * @param output: (output) The array of shares generated from the secret
 * @return 0 if successful
 */  
int S3ORAM::createShares(TYPE_DATA input, TYPE_DATA* output)
{
    unsigned long long random[PRIVACY_LEVEL];
    for ( int i = 0 ; i < PRIVACY_LEVEL ; i++)
    {
    #if defined(NTL_LIB)
        zz_p rand;
        NTL::random(rand);
        memcpy(&random[i], &rand,sizeof(TYPE_DATA));
    #else
        random[i] = Utils::_LongRand()+1 % P;
    #endif
    }
    for(unsigned long int i = 1; i <= NUM_SERVERS; i++)
    {
        output[i-1] = input;
        TYPE_DATA exp = i;
        for(int j = 1 ; j <= PRIVACY_LEVEL ; j++)
        {
            output[i-1] = (output[i-1] + Utils::mulmod(random[j-1],exp)) % P;
            exp = Utils::mulmod(exp,i);
	    }
    }
	
	return 0;
}


/**
 * Function Name: getSharedVector
 *
 * Description: Creates shares for NUM_SERVERS of servers from 1D array of logic values
 * 
 * @param logicVector: (input) 1D array of logical values
 * @param sharedVector: (output) 2D array of shares from array input
 * @return 0 if successful
 */  
int S3ORAM::getSharedVector(TYPE_DATA* logicVector, TYPE_DATA** sharedVector)
{
	cout << "	[S3ORAM] Starting to Retrieve Block Shares from Servers" << endl;

	TYPE_DATA outputVector[NUM_SERVERS];

	for (TYPE_INDEX i = 0; i < (H+1)*BUCKET_SIZE; i++)
	{
		createShares(logicVector[i],outputVector);
		for (int j = 0; j < NUM_SERVERS; j++){
			sharedVector[j][i] = outputVector[j];
		}
	}
	
	return 0;
}


/**
 * Function Name: simpleRecover
 *
 * Description: Recovers the secret from NUM_SERVERS shares by using first row of Vandermonde matrix
 * 
 * @param shares: (input) Array of shared secret as data chunks
 * @param result: (output) Recovered secret from given shares
 * @return 0 if successful
 */  
int S3ORAM::simpleRecover(TYPE_DATA** shares, TYPE_DATA* result)
{
	
    for(int i = 0; i < NUM_SERVERS; i++)
    {
        for(unsigned int k = 0; k < DATA_CHUNKS; k++)
        {
            result[k] = (result[k] + Utils::mulmod(vandermonde[i],shares[i][k])) % P; 
        }
    
	}

	cout << "	[S3ORAM] Recovery is Done" << endl;
	
	return 0;
}


/**
 * Function Name: precomputeShares
 *
 * Description: Creates several shares from an input based on Shamir's Secret Sharing algorithm
 * for precomputation purposes
 * 
 * @param input: (input) The secret to be shared
 * @param output: (output) 2D array of shares generated from the secret (PRIVACY_LEVEL x output_size)
 * @param output_size: (output) The size of generated shares from the secret
 * @return 0 if successful
 */  
int S3ORAM::precomputeShares(TYPE_DATA input, TYPE_DATA** output, TYPE_INDEX output_size)
{
    unsigned long long random[PRIVACY_LEVEL];
	cout << "=================================================================" << endl;
	cout<< "[S3ORAM] Precomputing Shares for " << input << endl;
    boost::progress_display show_progress(output_size);
    
	for(int k = 0; k < output_size; k++){
		for ( int i = 0 ; i < PRIVACY_LEVEL ; i++)
		{
        #if defined (NTL_LIB)
            zz_p rand;
            NTL::random(rand);
            memcpy(&random[i], &rand,sizeof(TYPE_DATA));
        #else
            random[i] = Utils::_LongRand()+1 % P;
		#endif
        }
		for(unsigned long int i = 1; i <= NUM_SERVERS; i++)
		{
			output[i-1][k] = input;
			for(int j = 1 ; j <= PRIVACY_LEVEL ; j++)
			{
				output[i-1][k] = (output[i-1][k] + Utils::mulmod(random[j-1],i)) % P;
			}
		}
		++show_progress;
	}
	
	return 0;
}



