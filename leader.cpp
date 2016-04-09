#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <random>

#define MIN_PORTNO 2000
#define MAX_PORTNO 50000

using namespace std;

class Leader {
	
	socklen_t len; //store size of the address
	int portNo, socketFd; // portNumber 
	struct sockaddr_in svrAdd, clientAdd;
	string name;
	public:
		Leader(string leaderName); 
		int startServer();

}

Leader:: Leader(string leaderName) {
	name = leaderName;
	startServer();	
}

// Return port number of the UDP server
int Leader:: startServer(){
    // create UDP socket
    socketFd = socket(AF_INET, SOCK_DGRAM, 0);

    bzero((char*) &svrAdd, sizeof(svrAdd));

    svrAdd.sin_family = AF_INET;
    svrAdd.sin_addr.s_addr = INADDR_ANY;
 
   /** bind to the UDP socket on the random port number
	and retry with different random port if binding fails **/
    
    for(int i=0; i<30; i++) {

	int range = max - min + 1;
	portNo = rand() % range + min;
	svrAdd.sin_port = htons(portNo);
	svrLen = sizeof(svrAdd);
	
	if(bind(socketFd, (struct sockaddr *)&svrAdd, sizeof(svrAdd)) < 0) {
	        cerr << "Error: Cannot bind socket on " <<portNo<<endl;
    	}else
		break;

    }

    clientLen = sizeof(clientAdd);

    cout<<"Started Server on "<<portNo<<endl;

}


int main(int argc, char*argv[]) {
	    
	


}


