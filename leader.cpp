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
    sockFd = socket(AF_INET, SOCK_DGRAM, 0);
    bzero((char*) &sock, sizeof(sock));
    sock.sin_family = AF_INET;
    sock.sin_addr.s_addr = INADDR_ANY;
	port = generatePortNumber(sockFd, sock); */

    printStartMessage();
	
	// Start producer and consumer threads
	thread prod(&Leader::producerTask, this, sockFd);
	thread con(&Leader::consumerTask, this);	
	prod.join();
	con.join();	

	// create heartbeat socket
	heartbeatSockFd = socket(AF_INET, SOCK_DGRAM, 0);
    bzero((char*) &heartbeatSock, sizeof(heartbeatSock));
    heartbeatSock.sin_family = AF_INET;
    heartbeatSock.sin_addr.s_addr = INADDR_ANY;
	heartbeatPortNo = generatePortNumber(heartbeatSockFd, heartbeatSock);
	// Start heartbeat threads
	thread heartbeatSend(&Leader::heartbeatSender, this);
	thread heartbeatRecv(&Leader::producerTask, this, heartbeatSockFd);
	thread detectFailure(&Leader::detectClientFaliure, this);
	heartbeatSend.join();
	heartbeatRecv.join();
	detectFailure.join();
	
	// create ACK socket
	ackSockFd = socket(AF_INET, SOCK_DGRAM, 0);
    bzero((char*) &ackSock, sizeof(ackSock));
    ackSock.sin_family = AF_INET;
    ackSock.sin_addr.s_addr = INADDR_ANY;
	ackPort = generatePortNumber(ackSockFd, ackSock);
	

}

/*  Print welcome message on start of UDP server
*/
void Leader::printStartMessage() {
	cout<<name<<" started a new chat, listening on "<<ip<<":"<<port<<endl;
	cout<<"Succeeded, current users:"<<endl;
	cout<<name<<" "<<ip<<":"<<port<<"(Leader)"<<endl;
	if(chatRoom.empty()) {
		cout<<"Waiting for others to join..."<<endl;
	}else{
		map<Id, ChatRoomUser>::iterator it;
		for(it = chatRoom.begin(); it != chatRoom.end(); it++) {
 		   	// iterator->first = key
			// iterator->second = value
			cout<<it->second->name<<" "<<it->first<<endl;
		}	
	}
}

/*   Parse the encoded message into a Message object
     and order using sequence number and add to Queue
*/
void Leader::parseMessage(char *message, Id clientId) {
	
	// spliting the encoded message
	vector<string> messageSplit;
	boost::split(messageSplit,message,boost::is_any_of("%"));
    
	int messageType = atoi(messageSplit[0].c_str());
	switch(messageType) {
		case JOIN:        // Format: JOIN%userName%ip:portNo%ackPortNum%heartBeatPortNum
			{
				// Add new user to map 
				string user = messageSplit[1];
				vector<string> ipPortSplit;
				boost::split(ipPortSplit,messageSplit[2],boost::is_any_of(":"));
				string ip = ipPortSplit[0];
				int port = atoi(ipPortSplit[1].c_str());
				int ackPort = atoi(messageSplit[3].c_str());
				int heartbeatPort = atoi(messageSplit[4].c_str());
				chatRoom[clientId] = ChatRoomUser(user, ip, port, ackPort, heartbeatPort);
				
				sendListUsers(clientId.ip, clientId.port); 
				
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
				Message responseObj = Message(CHAT,++seqNum, chatRoom[clientId].name + ":: " + messageSplit[1]);
				q.push(responseObj);
			}
			break;
		case DELETE:
			{
				deleteUser(clientId);	
			}	
			break;
		case HEARTBEAT:
			{
				// Format: 3%clientIp:clientPort
				chatRoom[clientId].lastHbt = chrono::system_clock::now();
			}
			break;
		default:
			cout<<"Invalid chat code sent by participant"<<endl;

	}
	   	
}

void deleteUser(Id clientId) {
	// Delete user from map, ackMap, heartBeatMap
	string user = chatRoom[clientId].name;
	chatRoom.erase(clientId);
		
	#ifdef DEBUG
	cout<<"[DEBUG]Deleting "<<user<<endl;
	#endif
	// add NOTICE message to Queue	
	stringstream response;	
	response << clientId << "%NOTICE " << user << " left the chat or just crashed";
	Message responseObj = Message(DELETE, ++seqNum, response.str());
	q.push(responseObj);	
	
	
}

/*  Producer thread constantly listening to 
    messages on the socket specified by fd
    and parses the message to take appropriate 
    action
*/

void Leader::producerTask(int fd) {
	#ifdef DEBUG
	cout<<"[DEBUG] Producer thread started"<<endl;
	#endif
	while (true) {
		struct sockaddr_in clientAdd;
		socklen_t clientLen = sizeof(clientAdd);
       	char readBuffer[500];
        bzero(readBuffer, 501);
		        
        recvfrom(fd, readBuffer, 500, 0, (struct sockaddr *) &clientAdd, &clientLen);
		char clientIp[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(clientAdd.sin_addr), clientIp, INET_ADDRSTRLEN);	
		
		#ifdef DEBUG
        	cout<<"[DEBUG] Received from " <<clientIp<<":"<<ntohs(clientAdd.sin_port)<<" - "<<readBuffer<<endl;
		#endif

        	// parseMessage(readBuffer,string(clientIp), ntohs(clientAdd.sin_port));
		parseMessage(readBuffer, getId());
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
		Message m = q.pop();
			
		// Multi-cast to everyone in the group 
		map<Id, ChatRoomUser>::iterator it;
        for(it = chatRoom.begin(); it != chatRoom.end(); it++) {
			stringstream msgStream;
			msgStream << m.getType() << "%" << m.getMessage();
			string msg = msgStream.str();
			#ifdef DEBUG
				cout<<"[DEBUG]Sending "<<msg<<" to "<<it->second->name<<"@"<<it->first<<endl;
			#endif
			cout<<msg<<endl;
			string clientIp = it->first.ip;
			int clientPort = it->first.port;
 
			/* set socket attributes for participant */
			struct sockaddr_in clientAdd;
			bzero((char *) &clientAdd, sizeof(clientAdd));
			clientAdd.sin_family = AF_INET;
			inet_pton(AF_INET,clientIp.c_str(),&(clientAdd.sin_addr));
			clientAdd.sin_port = htons(clientPort);
			
			sendMessageWithRetry(sockFd, msg.c_str(),msg.length(),0, (struct sockaddr *) &clientAdd, sizeof(clientAdd));
		}		
	}	
}

void Leader::sendListUsers(string clientIp, int clientPort) {
	map<Id, ChatRoomUser>::iterator it;
	stringstream ss;
	ss << LIST_OF_USERS << "%" << name << "&" << ip << ":" << port <<" (Leader)"<< "&" << ackPort << "&" << heartbeatPort << "%";
	for(it = chatRoom.begin(); it != chatRoom.end(); it++) {
		ss << it->second->name << "&" << it->first << "%";		
	}
	
	/* set socket attributes for participant */
	struct sockaddr_in clientAdd;
	bzero((char *) &clientAdd, sizeof(clientAdd));
	clientAdd.sin_family = AF_INET;
	inet_pton(AF_INET,clientIp.c_str(),&(clientAdd.sin_addr));
	clientAdd.sin_port = htons(clientPort);

	string response = ss.str();
	#ifdef DEBUG
	cout<<"[DEBUG]Sending List of users to "<<clientIp<<":"<<clientPort<<" "<<response<<endl;
	cout<<"[DEBUG] Response length: "<< response.length()<<endl;
	#endif
	sendto(sockFd, response.c_str(), response.length(), 0, (struct sockaddr *) &clientAdd, sizeof(clientAdd));
}


// to send recieve heartbeats, detect failures
void Leader::heartbeatSender(){
	while(true){	
			
		// thread sleep
		this_thread::sleep_for(std::chrono::milliseconds(HEARTBEAT_THRESHOLD));		
	}
}


void Leader::detectClientFaliure(){
	while(true){
		chrono::time_point<chrono::system_clock> curTime;
		curTime = chrono::system_clock::now();
		map<Id, ChatRoomUser>::iterator it;
		for(it = clientBeat.begin(); it != clientBeat.end(); it++){
			chrono::time_point<chrono::system_clock> beatTime = it->second->lastHbt;
			chrono::duration<double> elapsedSeconds = beatTime - curTime;

			// edit HEARTBEAT_THRESHOLD to required metric
			chrono::duration<double> thresholdSeconds =  chrono::duration<double>(HEARTBEAT_THRESHOLD * 0.002);
			if (elapsedSeconds > thresholdSeconds){
				// Node failure
				deleteUser(it->first);
			} 
		}
		// thread sleep
		this_thread::sleep_for(std::chrono::milliseconds(HEARTBEAT_THRESHOLD));
	}
}
