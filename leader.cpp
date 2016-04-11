#include <iostream>
#include "globals.h"
#include "utils.h"
#include <sstream> 
#include<sys/time.h>
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
	
	// seed the random number generator with curent time    
	struct timeval t1;
	gettimeofday(&t1, NULL);
	srand(t1.tv_usec * t1.tv_sec);
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
	thread prod(&Leader::producerTask, this);
	thread con(&Leader::consumerTask, this);	
	prod.join();
	con.join();
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
     and order using sequence number and add to Queue
*/
void Leader::parseMessage(char *message, string clientIp, int clientPort) {
	stringstream clientIdStream;
	clientIdStream<<clientIp<<":"<<clientPort;
	string clientId = clientIdStream.str();

	// spliting the encoded message
    	vector<string> messageSplit;
    	boost::split(messageSplit,message,boost::is_any_of("%"));
    
	int messageType = atoi(messageSplit[0].c_str());
	switch(messageType) {
		case JOIN:        // Format:  1%user%ip:port
			{
				// Add new user to map 
				string user = messageSplit[1];
				chatRoom[clientId] = user;
				
				// Split IP and Port and send list of users in chatroom
				/* vector<string> ipPortSplit;
    				boost::split(ipPortSplit,messageSplit[2],boost::is_any_of(":"));
				string clientIp = ipPortSplit[0];
				int clientPort = atoi(ipPortSplit[1].c_str());*/
				sendListUsers(clientIp, clientPort); 
			
				// add NOTICE message to Queue	
				stringstream response;	
				response << "NOTICE " << user << " joined on " << clientId;
				Message responseObj = Message(CHAT, ++seqNum, response.str());
				q.push(responseObj);		
			}			
			break;
		case CHAT:
			{
				/**** Ordering of messages ****/
				Message responseObj = Message(CHAT,++seqNum, chatRoom[clientId] + ":: " + messageSplit[1]);
				q.push(responseObj);
			}
			break;
		case DELETE:
			{
				// Delete user from map
				string ipPort = messageSplit[1];
				string user = chatRoom[ipPort];
				chatRoom.erase(ipPort); 
				
				sendListUsers(clientIp, clientPort); 
			
				// add NOTICE message to Queue	
				stringstream response;	
				response << CHAT << "%NOTICE " << user << " left the chat or just crashed";
				Message responseObj = Message(CHAT, ++seqNum, response.str());
				q.push(responseObj);		

			}	
			break;
		default:
			cout<<"Invalid chat code sent by participant"<<endl;

	}
	   	
}

/*  Producer thread constantly listening to 
    messages and enqueues them
*/

void Leader::producerTask() {
	#ifdef DEBUG
	cout<<"[DEBUG] Producer thread started"<<endl;
	#endif
	while (true) {
		struct sockaddr_in clientAdd;
		socklen_t clientLen = sizeof(clientAdd);
        	char readBuffer[500];
        	bzero(readBuffer, 501);
		        
        	recvfrom(socketFd, readBuffer, 500, 0, (struct sockaddr *) &clientAdd, &clientLen);
		char clientIp[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(clientAdd.sin_addr), clientIp, INET_ADDRSTRLEN);	
		
		#ifdef DEBUG
        	cout<<"[DEBUG] Received from " <<clientIp<<":"<<ntohs(clientAdd.sin_port)<<" - "<<readBuffer<<endl;
		#endif

        	parseMessage(readBuffer,string(clientIp), ntohs(clientAdd.sin_port));
	}
}

/*   Worker/ Consumer thread constantly dequeuing messages
     and multi-casting it to participants in the chatroom
*/
void Leader::consumerTask() {
	#ifdef DEBUG
	cout<<"[DEBUG]Consumer thread started"<<endl;
	#endif	
	while(true) {
		// TODO: wait for ACKs from everyone
	
		/* struct sockaddr_in clientAdd;
		socklen_t clientLen = sizeof(clientAdd);
        	char readBuffer[500];
        	bzero(readBuffer, 501);
	
		recvfrom(socketFd, readBuffer, 500, 0, (struct sockaddr *) &clientAdd, &clientLen);
		#ifdef DEBUG
        	cout<<"[DEBUG] Received from Client:" << readBuffer<<endl;
		#endif
		*/ 
		
		Message m = q.pop();
			
		// TODO: Multi-cast to everyone in the group 
		map<string,string>::iterator it;
                for(it = chatRoom.begin(); it != chatRoom.end(); it++) {
			stringstream msgStream;
			msgStream << m.getType() << "%" << m.getMessage();
			string msg = msgStream.str();
			#ifdef DEBUG
                        cout<<"Sending "<<msg<<" to "<<it->second<<"@"<<it->first<<endl;
			#endif
			
			vector<string> messageSplit;
        		boost::split(messageSplit,it->first,boost::is_any_of(":"));
			string clientIp = messageSplit[0];
		        int clientPort = atoi(messageSplit[1].c_str());
 
			/* set socket attributes for participant */
			struct sockaddr_in clientAdd;
			bzero((char *) &clientAdd, sizeof(clientAdd));
        		clientAdd.sin_family = AF_INET;
        		inet_pton(AF_INET,clientIp.c_str(),&(clientAdd.sin_addr));
        		clientAdd.sin_port = htons(clientPort);
			
			sendto(socketFd, msg.c_str(),msg.length(),0, (struct sockaddr *) &clientAdd, sizeof(clientAdd));
                }		
	}	
}

void Leader::sendListUsers(string clientIp, int clientPort) {
	map<string,string>::iterator it;
	stringstream ss;
	ss << LIST_OF_USERS << "%" << name << "&" << ip << ":" << portNo << " (Leader) %";
	for(it = chatRoom.begin(); it != chatRoom.end(); it++) {
		ss << it->second << "&" << it->first << "%";		
	}
	
	/* set socket attributes for participant */
	struct sockaddr_in clientAdd;
	bzero((char *) &clientAdd, sizeof(clientAdd));
	clientAdd.sin_family = AF_INET;
	inet_pton(AF_INET,clientIp.c_str(),&(clientAdd.sin_addr));
	clientAdd.sin_port = htons(clientPort);

	string response = ss.str();
	#ifdef DEBUG
	cout<<"Sending List of users to "<<clientIp<<":"<<clientPort<<" "<<response<<endl;
	cout<<response.length()<<endl;
	#endif
	sendto(socketFd, response.c_str(), response.length(), 0, (struct sockaddr *) &clientAdd, sizeof(clientAdd));

}
