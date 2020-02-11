#include <iostream>
#include "config.h"
#include "ClientS3ORAM.hpp"
#include "ServerS3ORAM.hpp"



#if defined(CORAM_LAYOUT)
        #include "ServerKaryS3ORAMC.hpp"
        #include "ClientKaryS3ORAMC.hpp"
#else
    #if defined(TRIPLET_EVICTION)
        #include "ServerBinaryS3ORAMO.hpp"
        #include "ClientBinaryS3ORAMO.hpp"
    #else
        #include "ServerKaryS3ORAMO.hpp"
        #include "ClientKaryS3ORAMO.hpp"
    #endif
    
#endif

#include "Utils.hpp"

using namespace std;


#include <thread>

unsigned int nthreads = std::thread::hardware_concurrency();

#include "S3ORAM.hpp"


int main(int argc, char **argv)
{    
    string mkdir_cmd = "mkdir -p ";
    string mkdir_localState = mkdir_cmd + clientLocalDir;
    string mkdir_unsharedData = mkdir_cmd + clientDataDir;
    string mkdir_log = mkdir_cmd + logDir;
    
    system(mkdir_localState.c_str());
    system(mkdir_unsharedData.c_str());
    system(mkdir_log.c_str());
    for(int i = 0 ; i < NUM_SERVERS; i++)
    {
        string mkdir_sharedData = mkdir_cmd +  rootPath + to_string(i);
        system(mkdir_sharedData.c_str());
    }

    int choice;
    zz_p::init(P);
    //set random seed for NTL
    ZZ seed = conv<ZZ>("123456789101112131415161718192021222324");
    SetSeed(seed);
    
	cout << "CLIENT(1) or SERVER(2): ";
	cin >> choice;
	cout << endl;
	
	if(choice == 2)
	{
		int serverNo;
        int selectedThreads;
		cout << "Enter the Server No (1-"<< NUM_SERVERS <<"):";
		cin >> serverNo;
		cin.clear();
		cout << endl;
        
        do
        {
            cout<< "How many computation threads to use? (1-"<<nthreads<<"): ";
            cin>>selectedThreads;
		}while(selectedThreads>nthreads);
        
#if defined(CORAM_LAYOUT)
        ServerS3ORAM*  server = new ServerKaryS3ORAMC(serverNo-1,selectedThreads);
#else
    #if defined(TRIPLET_EVICTION)
        ServerS3ORAM*  server = new ServerBinaryS3ORAMO(serverNo-1,selectedThreads);
    #else
        ServerS3ORAM* server = new ServerKaryS3ORAMO(serverNo-1,selectedThreads);
    #endif
#endif
        
        
        
		server->start();
	}
	else if (choice == 1)
	{
        
#if defined(CORAM_LAYOUT)
        ClientKaryS3ORAMC*  client = new ClientKaryS3ORAMC();
#else
    #if defined(TRIPLET_EVICTION)
        ClientS3ORAM* client = new ClientBinaryS3ORAMO();
    #else
        ClientS3ORAM* client = new ClientKaryS3ORAMO();
    #endif
#endif
		
        
        int access, start;
		char response = ' ';
		int random_access;
        int subOpt;
        cout<<"LOAD PREBUILT DATA (1) OR CREATE NEW ORAM (2)? "<<endl;
        cin>>subOpt;
        cout<<endl;
        if(subOpt==1)
        {
            client->loadState();
        }
        else
        {
            client->init();
            do
            {
                cout << "TRANSMIT INITIALIZED S3ORAM DATA TO NON-LOCAL SERVERS? (y/n)";
                cin >> response;
                response = tolower(response);
            }
            while( !cin.fail() && response!='y' && response!='n' );
            if (response == 'y')
            {
                client->sendORAMTree();
            }
		    
        }
		cout << "SERVERS READY? (Press ENTER to Continue)";
		cin.ignore();
		cin.ignore();
		cin.clear();
		cout << endl<<endl<<endl;
        for(int j = 0 ; j < 10 ; j++)
        {
            for(int i = 1 ; i < NUM_BLOCK+1; i++)
            {
                client->access(i);
            }
        }
        cout<<"Done!";
    }
	else
	{
		cout << "COME ON!!" << endl;
	}
    
    return 0;
}
