/*
 * config.h
 *
 *  Created on: Mar 2, 2017
 *      Author: ceyhunozkaptan, thanghoang
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include "vector"
#include <math.h>
#include "fstream"
#include <iostream>
#include "stdio.h"
#include "stdlib.h"
#include <cstring>
#include <bitset>
#include <algorithm>
#include <assert.h>
#include <boost/progress.hpp>
#include <map>
#include <chrono>

#include <string>
#include <sstream>
#include <bitset>

template <typename T>
static inline std::string to_string(T value)
{
    std::ostringstream os ;
    os << value ;
    return os.str() ;
}


static const unsigned long long P = 1073742353; //prime field - should have length equal to the defined TYPE_DATA


#define CORAM_LAYOUT
//#define TRIPLET_EVICTION
#define K_ARY 3



#define NTL_LIB //disable it if compiled for android
//=== PARAMETERS ============================================================
#define BLOCK_SIZE 16
#define HEIGHT 3


#if defined(CORAM_LAYOUT)
    #define BUCKET_SIZE (K_ARY)
    #define EVICT_RATE 1
    const unsigned long long NUM_BLOCK = pow(K_ARY,HEIGHT);
    const unsigned long long NUM_NODES = (int) (pow(K_ARY,HEIGHT+1)-1)/(K_ARY-1);
#else
    #if defined (TRIPLET_EVICTION)
        #define K_ARY 2
        #define BUCKET_SIZE 74 
        #define EVICT_RATE 37
        const unsigned long long NUM_BLOCK = ((int) (pow(2,HEIGHT-1))*EVICT_RATE-1);
        const unsigned long long  NUM_NODES = (int) (pow(2,HEIGHT+1)-1);
    #else
        #define BUCKET_SIZE 600 
        #define EVICT_RATE (BUCKET_SIZE/2)
        const unsigned long long NUM_BLOCK = ((pow(K_ARY,HEIGHT-1))*EVICT_RATE-1);
        const unsigned long long NUM_SLIDES = K_ARY; //should be divisible with BUCKET_SIZE
        const unsigned long long NUM_NODES = (int) (pow(K_ARY,HEIGHT+1)-1)/(K_ARY-1) + pow(K_ARY,HEIGHT); 
    #endif
#endif

#define STASH_SIZE 80

const int H = HEIGHT; 
typedef unsigned long long TYPE_DATA;


//=== MODES ==================================================================
//#define PRECOMP_MODE //for shares of *0* and *1* precomputation
//============================================================================


//=== SECRET SHARING PARAMETER================================================
#define NUM_SERVERS 3
#define PRIVACY_LEVEL 1
const long long int vandermonde[NUM_SERVERS] = {3 , -3 + P, 1};
//const long long int vandermonde[NUM_SERVERS] = {7, -21+P, 35, -35+P, 21, -7+P, 1};

/** Vandermonde Values for Different Number of Servers (7, 5, 3)*/
//{7, -21+P, 35, -35+P, 21, -7+P, 1};//{5, -10+P, 10, -5+P, 1};//{3, -3+P, 1};
//============================================================================


//=== SERVER INFO ============================================================

	//SERVER IP ADDRESSES
const std::string SERVER_ADDR[NUM_SERVERS] = {"tcp://localhost", "tcp://localhost", "tcp://localhost"};//, "tcp://localhost", "tcp://localhost", "tcp://localhost", "tcp://localhost"}; 	
#define SERVER_PORT 25555        //define the first port to generate incremental ports for client-server /server-server communications

//============================================================================


//=== PATHS ==================================================================
const std::string rootPath = "../data/";

const std::string clientLocalDir = rootPath + "client_local/";
const std::string clientDataDir = rootPath + "client/";

const std::string clientTempPath = rootPath + "client_local/local_data";

const std::string logDir = "../" + to_string(H) + "_" + to_string(BLOCK_SIZE) + "/" + "log/";
//=============================================================================


//=== NON-MODIFIABLE PARAMETER ================================================
#if defined(NTL_LIB)
    #include "NTL/ZZ.h"
    #include "NTL/tools.h"
    #include "NTL/GF2E.h"
    #include <NTL/WordVector.h>
    #include <NTL/vector.h>
    #include "NTL/lzz_p.h"
    #include "NTL/ZZ_p.h"
    using namespace NTL;
#endif

using namespace std;

#define time_now std::chrono::high_resolution_clock::now()
//=============================================================================


//=== DATA TYPE ===============================================
typedef unsigned long long TYPE_ID;
typedef long long int TYPE_INDEX;
typedef struct type_pos_map
{
	TYPE_INDEX pathID;
	TYPE_INDEX pathIdx;
}TYPE_POS_MAP;


#define DATA_CHUNKS BLOCK_SIZE/sizeof(TYPE_DATA)
const TYPE_INDEX PRECOMP_SIZE = BUCKET_SIZE*(2*HEIGHT+1)*BUCKET_SIZE*(2*HEIGHT+1);

const TYPE_INDEX N_leaf = pow(K_ARY,H);


#if defined(CORAM_LAYOUT)
    const TYPE_INDEX evictMatSize = (BUCKET_SIZE+1)*(BUCKET_SIZE+1);
#else
    #if defined(TRIPLET_EVICTION)
        const TYPE_INDEX evictMatSize = 2*BUCKET_SIZE*BUCKET_SIZE;
    #else
        const TYPE_INDEX evictMatSize = BUCKET_SIZE*BUCKET_SIZE;
    #endif
#endif
//====================================================================



//=== SOCKET COMMAND =========================================
#define CMD_SEND_ORAM_TREE        		0x000010
#define CMD_SEND_BLOCK		          	0x00000E 

#define CMD_SEND_EVICT			        0x000040
#define CMD_REQUEST_BLOCK		        0x000050
#define CMD_START_REDUC			        0x000052
#define CMD_SUCCESS                     "OK"
//============================================================


#endif /* CONFIG_H_ */
