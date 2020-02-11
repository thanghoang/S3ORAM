# Basic S3ORAM 
Basic implementation of S3ORAM appeared in CCS'17 ([free version](https://eprint.iacr.org/2017/819.pdf)). This project is built on [CodeLite IDE](http://codelite.org). It is recommended to install CodeLite to load the full S3ORAM workspace. 


# S3ORAM Extension ([ACM TOPS'20](https://dl.acm.org/doi/abs/10.1145/3369108)) is now available. Check out the new branch [here](https://github.com/thanghoang/S3ORAM/tree/Extension)!!

## Updates
* 2017-12-25: S3ORAM now supports more than 3 servers with higher privacy levels.


# Required Libraries
1. [NTL v9.10.0](http://www.shoup.net/ntl/download.html)

2. [ZeroMQ](http://zeromq.org/intro:get-the-software)

# Configuration
All S3ORAM configurations are located in ```S3ORAM/config.h```. 

## Highlighted Parameters:
```
#define BLOCK_SIZE 128                                -> Block size (in bytes)

#define HEIGHT 4                                      -> Height of S3ORAM Tree

#define BUCKET_SIZE 333                               -> Bucket size

#define EVICT_RATE 280                                -> Eviction frequency

static const unsigned long long P = 1073742353;       -> Prime field (size should be equal to the defined TYPE_DATA)

#define NUM_SERVERS 7                                 -> Number of servers \ell.
#define PRIVACY_LEVEL 3                               -> Privacy level t. 

const long long int vandermonde[NUM_SERVERS]          -> The first row of inverse of vandermonde matrix (should be defined according to SERVER_ID from 1....n)

const std::string SERVER_ADDR[NUM_SERVERS]            -> Server IP addresses
#define SERVER_PORT 5555                              -> Define the first port for incremental to generate other ports for client-server / server-server communications

```

### Notes
Due to the imperfection of PRF, it is recommended to select ```BUCKET_SIZE``` larger than ```EVICT_RATE``` to avoid bucket overflow.

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
Since android device is resource-limited, it is recommended to generate ORAM data using resourceful client machine (e.g., desktop/laptop), and then copy generated client local data (``S3ORAM/data/client_local/`` folder) to the android device for running the experiment. Here the instruction to generate android executable file:


1. Download [Android NDK](https://developer.android.com/ndk/downloads/index.html)
2. Add the location of ``ndk-build`` file (i.e., in ``$android-ndk-path/build/``) to ``PATH`` environment variable via e.g.,

``` 
1. vim ~/.profile
2. add `` export PATH=$PATH:$android-ndk-path/build/`` at the end of the file, where $android-ndk-path is the (absolute) path of the android NDK
3. save and reload the profile
```

3. Goto folder ``android-jni/jni``. Open the file ``Android.mk`` and fix ``SRC_PATH`` and ``NDK_PATH`` variables, where ``SRC_PATH`` is the absolute path to  the  S3ORAM src code (e.g., $home/S3ORAM/S3ORAM/), and ``NDK_PATH`` is the absolute path to the Android NDK (e.g., $home/$android-ndk-path).
4. Disable ``NTL_LIB`` macro in file ``S3ORAM/config.h``.
5. Goto folder ``android-jni/jni`` , execute ``ndk-build`` command, which will generate executable file ``s3oram_client`` and library file ``libgnustl_shared.so`` in ``android-jni/libs/armeabi/`` folder.
6. Create a folder named ``s3oram`` in the android phone (via ``adb shell``). Inside ``s3oram`` folder, create a folder named ``bin``.
7. Copy files ``s3oram_client`` and ``libgnustl_shared.so`` to folder ``s3oram/bin/`` in the android phone via ``adb push``. Copy S3ORAM client data folder (i.e., ``S3ORAM/data/client_local/``) to ``s3oram/`` in the android phone.
8. Access to the Android phone via ``adb shell``. Set the LD_LIBRARY_PATH to ``s3oram/bin/`` 
 ```
export LD_LIBRARY_PATH=$home/s3oram/bin
```
9. Locate to folder ``s3oram/bin``, run the file ``s3oram_client`` to run the program.

# Further Information
For any inquiries, bugs, and assistance on building and running the code, please contact me at [hoangm@mail.usf.edu](mailto:hoangm@mail.usf.edu?Subject=[BasicS3ORAM]%20Inquiry).
