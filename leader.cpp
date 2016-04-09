#include <iostream>
#include "globals.h"
#include "utils.h"
// Range of random port numbers where UDP server starts
#define MIN_PORTNO 2000
#define MAX_PORTNO 50000

using namespace std;

// define chat codes
const int JOIN = 1;
const int DELETE = 2; 



Leader :: Leader(string leaderName) {
	name = leaderName;
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
}

void Leader::printStartMessage() {
	cout<<name<<" started a new chat, listening on "<<ip<<":"<<portNo<<endl;
	cout<<"Succeeded, current users:"<<endl;
	cout<<name<<inet_ntoa(svrAdd.sin_addr)<<":"<<ntohs(svrAdd.sin_port)<<"(Leader)"<<endl;
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
