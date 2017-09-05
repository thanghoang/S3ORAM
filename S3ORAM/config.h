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


#define NTL_LIB //disable it if compiled for android
//=== PARAMETERS ============================================================
#define BLOCK_SIZE 128
#define HEIGHT 4
#define BUCKET_SIZE 333 
#define EVICT_RATE 280
const int H = HEIGHT; 

static const unsigned long long P = 1073742353; //prime field - should have length equal to the defined TYPE_DATA
typedef unsigned long long TYPE_DATA;


//=== MODES ==================================================================
//#define PRECOMP_MODE //for shares of *0* and *1* precomputation
//============================================================================


//=== SECRET SHARING PARAMETER================================================
#define NUM_SERVERS 3
#define PRIVACY_LEVEL 1 
const long long int vandermonde[NUM_SERVERS] = {3, -3+P, 1};
//============================================================================


//=== SERVER INFO ============================================================
const std::string SERVER_ADDR[NUM_SERVERS] = {"tcp://localhost", "tcp://localhost", "tcp://localhost"}; 	// SERVER IP ADDRESSES for CLIENT COMM
const std::string SERVER_PORT[NUM_SERVERS] = {"5555", "5556", "5557"};										// SERVER PORTS for CLIENT COMM

const std::string SERVER_RECV_PORT[2*NUM_SERVERS] = {"5558", "5559", "5560", "5561", "5562", "5563"};		// SERVER PORTS for INTER-SERVER COMM
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
const TYPE_INDEX N_leaf = pow(2,H);
const TYPE_INDEX NUM_BLOCK = ((int) (pow(2,HEIGHT-1))*EVICT_RATE-1);
const TYPE_INDEX NUM_NODES = (int) (pow(2,HEIGHT+1)-1);

const TYPE_INDEX evictMatSize = 2*BUCKET_SIZE*BUCKET_SIZE;
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
