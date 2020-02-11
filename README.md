# S3ORAM Extension
This branch contains the implementation of the **EXTENDED S3ORAM framework** from the basic S3ORAM scheme in CCS'17 that supports k-ary tree and Circuit-ORAM layout. The full paper is available [here](https://dl.acm.org/doi/abs/10.1145/3369108) (Feel free to [email me](mailto:hoangm@mail.usf.edu?subject=[Paper%20Request]%20S3ORAM%20ACM%20TOPS) for the free version).


This project is built on [CodeLite IDE](http://codelite.org). It is recommended to install CodeLite to load the full S3ORAM workspace. 

## Updates

* 2018-12-30: S3ORAM now has a new scheme that leverages the eviction paradigm in Circuit-ORAM. S3ORAM now also supports k-ary tree layout.
* 2017-12-25: S3ORAM now supports more than 3 servers with higher privacy levels.


# Required Libraries
1. [NTL v9.10.0](http://www.shoup.net/ntl/download.html)

2. [ZeroMQ](http://zeromq.org/intro:get-the-software)

# Configuration
All S3ORAM Framework configurations are located in ```S3ORAM/config.h```. 


## To use Circuit-ORAM eviction paradigm
Enable the macro ``#define CORAM_LAYOUT`` in ```S3ORAM/config.h```. 

## To use k-ary tree Layout
Modify the value in the macro ``#define K_ARY`` in ```S3ORAM/config.h```. 

## To use the default S3ORAM in CCS'17
Disable the macro ``#define CORAM_LAYOUT`` and enable the macro ``#define TRIPLET_EVICTION`` in ```S3ORAM/config.h```. 


## Highlighted Parameters:
```

#define BLOCK_SIZE 128                                -> Block size (in bytes)

#define HEIGHT 4                                      -> Height of S3ORAM Tree

static const unsigned long long P = 1073742353;       -> Prime field (size should be equal to the defined TYPE_DATA)

#define NUM_SERVERS 7                                 -> Number of servers \ell.
#define PRIVACY_LEVEL 3                               -> Privacy level t. 

const long long int vandermonde[NUM_SERVERS]          -> The first row of inverse of vandermonde matrix (should be defined according to SERVER_ID from 1....n)

const std::string SERVER_ADDR[NUM_SERVERS]            -> Server IP addresses
#define SERVER_PORT 5555                              -> Define the first port for incremental to generate other ports for client-server / server-server communications

```



### Notes
It is recommended to select ```EVICT_RATE``` = ```BUCKET_SIZE/2```, and ```BUCKET_SIZE>73``` to avoid bucket overflow.

The folder ```S3ORAM/data``` is required to store generated S3ORAM data structure.

# Build & Compile
Goto folder ``S3ORAM/`` and execute
``` 
make
```

, which produces the binary executable file named ```S3ORAM``` in ``S3ORAM/Debug/``.

# Usage

Run the binary executable file ```S3ORAM```, which will ask for either Client or Server mode. The S3ORAM implementation can be tested using either **single** machine or **multiple** machines:


## Local Testing:
1. Set ``SERVER_ADDR`` in ``S3ORAM/config.h`` to be ``localhost``. 
2. Choose unique ``SERVER_PORT`` and ``SERVER_RECV_PORT`` for each server entity. 
3. Compile the code with ``make`` in the ``S3ORAM/`` folder. 
4. Go to ``S3ORAM/Debug`` and run the compiled ``S3ORAM`` file in different Terminals, each playing the client/server role.

## Real Network Testing:
1. Copy the binary file ``S3ORAM`` compiled under the same configuration to running machines. 
2. For first time usage, run the ``S3ORAM/Debug/S3ORAM`` file on the *client* machine to initialize the S3ORAM structure first.
3. Copy the folder ``S3ORAM/data/i/`` to server *i*, or follow the instruction on the client machine to transmit all data to corresponding server (It is recommend to manually copy the folder to avoid interuption during tranmission).
4. For each server *i*, run the compiled file ``S3ORAM`` and select the server role (option 2) and the corresponding ID ``i``.


# Android Build
(to be updated)


## Citing

If the code is found useful, we would be appreciated if our paper can be cited with the following bibtex format 

```
@article{10.1145/3369108, 
author = {Hoang, Thang and Yavuz, Attila A. and Guajardo, Jorge}, 
title = {A Multi-Server ORAM Framework with Constant Client Bandwidth Blowup}, 
year = {2020}, 
issue_date = {February 2020}, 
publisher = {Association for Computing Machinery}, 
address = {New York, NY, USA}, 
volume = {23}, 
number = {1}, issn = {2471-2566}, 
url = {https://doi.org/10.1145/3369108}, doi = {10.1145/3369108}, 
journal = {ACM Trans. Priv. Secur.}, 
month = feb, 
articleno = {Article 1}, 
numpages = {35}, 
keywords = {ORAM, privacy-enhancing technologies, secret sharing} }
```


# Further Information
For any inquiries, bugs, and assistance on building and running the code, please contact me at [hoangm@mail.usf.edu](mailto:hoangm@mail.usf.edu?Subject=[Extended%20S3ORAM]%20Inquiry).
