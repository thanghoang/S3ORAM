#include <iostream>
#include "ClientS3ORAM.hpp"
#include "iostream"
#include "Utils.hpp"

using namespace std;


int main(int argc, char **argv)
{
    
	int choice;
    	
	
	cout << "SERVER READY? (Press ENTER to Continue)";
	cin.ignore();
	cin.ignore();
	cin.clear();
	cout << endl;
		
	ClientS3ORAM* client = new ClientS3ORAM();
		
		
	#if defined(SETUP_FROM_DISK) //================================================================================================
		
		cout << "CONTINUE WITH LOCAL DATA? (Press ENTER to Continue)";
		cin.ignore();
		cin.clear();
		cout << endl;
			
		client->load();
		
	#else //defined(SETUP_FROM_DISK) ================================================================================================
		
    cout << "INITIALIZE THE CLIENT? (Press ENTER to Continue)";
    cin.ignore();
    cin.clear();
    cout << endl;
			
    client->init();
		
    #endif //defined(SETUP_FROM_DISK) ================================================================================================
    cout << endl;
    cout << endl;
    
    cout << "WARM-UP(1) OR RANDOM(2) ACCESS?";
    cin >> choice;
    cout << endl;
    cout << endl;
		
    int access, start;
    int n = 3;
    char response;
    
    if(choice == 1)
    {
        cout << "START FROM?(1-" << NUM_BLOCK << ")";
        cin >> start;
        cout << endl;
        //Sequential Access
        for (int i = 0 ; i < n ; i++)
        {
            cout << "=================================================================" << endl;
            cout << "[main] ROUND " << i+1 << " IS STARTING!" <<endl;
            cout << "=================================================================" << endl;
            for(int j = start; j <= EVICT_RATE+1; j++)
            {
                client->access(j);
                
            }
            cout << "=================================================================" << endl;
            cout << "[main] ROUND " << i+1 << " IS COMPLETED!" <<endl;
            cout << "=================================================================" << endl;
        }
        
    }
    else if(choice == 2)
    {
        cout << "HOW MANY ACCESS?";
        cin >> access;
        
        for (int i = 0 ; i < n ; i++)
        {
            cout << "=================================================================" << endl;
            cout << "[main] ROUND " << i+1 << " IS STARTING!" <<endl;
            cout << "=================================================================" << endl;
            for(int j = 1; j <= access; j++)
            {
                client->access(Utils::RandBound(NUM_BLOCK)+1);
            }
            cout << "=================================================================" << endl;
            cout << "[main] ROUND " << i+1 << " IS COMPLETED!" <<endl;
            cout << "=================================================================" << endl;
        }
    }
    else
    {
        cout << "COME ON!!" << endl;
    }
    ////cout<<endl<<client->getStashSize()<<endl;
    cout << "Done!" << endl;
          
    
    return 0;
}
