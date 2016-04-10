#include <iostream>
#include "globals.h"
#include "utils.h"
#include <thread>

// Range of random port numbers where UDP server starts
#define MIN_PORTNO 2000
#define MAX_PORTNO 50000

using namespace std;


Leader :: Leader(string leaderName) {
	name = leaderName;
	ip = getIp();
	startServer();	
}

/* Start UDP server on a random unused port number 
   and invoke the producer and consumer threads
*/
void Leader :: startServer(){
	// create UDP socket
    	socketFd = socket(AF_INET, SOCK_DGRAM, 0);

    	bzero((char*) &svrAdd, sizeof(svrAdd));

    	svrAdd.sin_family = AF_INET;
    	svrAdd.sin_addr.s_addr = INADDR_ANY;
 	

   	/* bind to the UDP socket on the random port number
	and retry with different random port if binding fails */
	
	srand(); // seed the random number generator with curent time    
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
	
	// TODO: Start producer and consumer threads
}

/*  Print welcome message on start of UDP server
*/
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

/*   Parse the encoded message into a Message object
*/
Message Leader::parseMessage(char *message) {
	// spliting the encoded message
    	vector<string> messageSplit;
    	boost::split(messageSplit,message,boost::is_any_of("%"));
    
	int messageType = atoi(messageSplit[0].c_str());
	seqNum++; /**** Ordering of messages ****/
   	return Message(messageType,seqNum, message);  	
}

/*  Producer thread constantly listening to 
    messages and enqueues them
*/

void Leader::producerTask() {
	while (true) {
		struct sockaddr_in clientAdd;
		socklen_t clientLen = sizeof(clientAdd);
        	char readBuffer[500];
        	bzero(readBuffer, 501);
		        
        	recvfrom(socketFd, readBuffer, 500, 0, (struct sockaddr *) &clientAdd, &clientLen);
		#ifdef DEBUG
        	cout<<"[DEBUG] Received from Client:" << readBuffer<<endl;
		#endif
        	Message m = parseMessage(readBuffer);
		q.push(m);
    	}
}

/*   Worker/ Consumer thread constantly dequeuing messages
     and multi-casting it to participants in the chatroom
*/
void Leader::consumerTask() {
	while(true) {
		// TODO: wait for ACKs from everyone
		Message m = q.pop();
		
		// TODO: Multi-cast to everyone in the group 
		map<string,string>::iterator it;
                for(it = chatRoom.begin(); it != chatRoom.end(); it++) {
			string msg = m.getMessage();
			#ifdef DEBUG
                        cout<<"Sending "<<msg<<" to "it->second<<" "<<it->first<<endl;
			#endif
			
			vector<string> messageSplit;
        		boost::split(messageSplit,it->first,boost::is_any_of(":"));
			string ip = messageSplit[0];
		        int port = atoi(messageSplit[1].c_str());
 
			/* set socket attributes for participant */
			struct sockaddr_in clientAdd;
			bzero((char *) &clientAdd, sizeof(clientAdd));
        		clientAdd.sin_family = AF_INET;
        		inet_pton(AF_INET,ip.c_str(),&(clientAdd.sin_addr));
        		clientAdd.sin_port = htons(port);
			
			sendto(socketFd, msg.c_str(),msg.length(),0, (struct sockaddr *) &clientAdd, sizeof(clientAdd));
                }		
	}	
}
