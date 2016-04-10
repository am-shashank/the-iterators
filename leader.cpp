#include <iostream>
#include "globals.h"
#include "utils.h"
// Range of random port numbers where UDP server starts
#define MIN_PORTNO 2000
#define MAX_PORTNO 50000

using namespace std;


Leader :: Leader(string leaderName) {
	name = leaderName;
	ip = getIp();
	startServer();	
}

// Return port number of the UDP server
int Leader :: startServer(){
	// create UDP socket
    	socketFd = socket(AF_INET, SOCK_DGRAM, 0);

    	bzero((char*) &svrAdd, sizeof(svrAdd));

    	svrAdd.sin_family = AF_INET;
    	svrAdd.sin_addr.s_addr = INADDR_ANY;
 
   	/** bind to the UDP socket on the random port number
	and retry with different random port if binding fails **/
    
   	while(true) {

		int range = MAX_PORTNO - MIN_PORTNO + 1;
		portNo = rand() % range + MIN_PORTNO;
		svrAdd.sin_port = htons(portNo);
	
		if(bind(socketFd, (struct sockaddr *)&svrAdd, sizeof(svrAdd)) < 0) {
		        cerr << "Error: Cannot bind socket on " <<portNo<<endl;
    		}else
			break;

    	}
    	printStartMessage();
	
	while (true) {
		struct sockaddr_in clientAdd;
		socklen_t clientLen = sizeof(clientAdd);
        	char readBuffer[500];
        	bzero(readBuffer, 501);        
        	recvfrom(socketFd, readBuffer, 500, 0, (struct sockaddr *) &clientAdd, &clientLen);
        	cout<<"Received from Client:" << readBuffer<<endl;
        	// parseMessage(readBuffer);
    	}
}

void Leader::printStartMessage() {
	cout<<name<<" started a new chat, listening on "<<ip<<":"<<portNo<<endl;
	cout<<"Succeeded, current users:"<<endl;
	cout<<name<<" "<<ip<<":"<<portNo<<"(Leader)"<<endl;
	if(chatRoom.empty()) {
		cout<<"Waiting for others to join..."<<endl;
	}else{
		map<string,string>::iterator it;
		for(it = chatRoom.begin(); it != chatRoom.end(); it++) {
 		   	// iterator->first = key
    			// iterator->second = value
			cout<<it->second<<" "<<it->first<<endl;
		}	
	}
}
