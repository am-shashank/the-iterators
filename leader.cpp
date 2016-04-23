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
    port = generatePortNumber(sockFd, sock); 

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
    heartbeatPort = generatePortNumber(heartbeatSockFd, heartbeatSock);
    
    // Start heartbeat threads
    thread heartbeatSend(&Leader::heartbeatSender, this);
    thread heartbeatRecv(&Leader::heartbeatReceiver, this);
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
			cout<<it->second.name<<" "<<it->first<<endl;
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
		case JOIN:        // Format: JOIN%msgId%userName%ip:portNo%ackPortNum%heartBeatPortNum
			{
				// Add new user to map 
				string user = messageSplit[2];
				vector<string> ipPortSplit;
				boost::split(ipPortSplit,messageSplit[3],boost::is_any_of(":"));
				string ip = ipPortSplit[0];
				int port = atoi(ipPortSplit[1].c_str());
				int ackPort = atoi(messageSplit[4].c_str());
				int heartbeatPort = atoi(messageSplit[5].c_str());
				chatRoom[clientId] = ChatRoomUser(user, ip, port, ackPort, heartbeatPort);
				
				sendListUsers(clientId.ip, clientId.port); 
				
				// add NOTICE message to Queue	
				stringstream response;	
				response << user << "%" << ip << ":" << port << "%" << ackPort <<"%" << heartbeatPort;
				Message responseObj = Message(ADD_USER, ++seqNum, response.str());
				q.push(responseObj);		
			}			
			break;
		case CHAT:  	// Format: CHAT%msgId
			{
				// Check for duplicate Message
				int msgId = stoi(messageSplit[2]);
				if(lastSeenMsgIdMap.find(clientId) != lastSeenMsgIdMap.end()) {
					if(msgId <= lastSeenMsgIdMap[clientId])
						break;
				}
				lastSeenMsgIdMap[clientId] = msgId;
				/**** Ordering of messages ****/
				++seqNum;
				string chatMsg = chatRoom[clientId].name + ":: " + messageSplit[1] + "%"+ clientId.ip + ":" + to_string(clientId.port) + "%" + to_string(msgId) + to_string(seqNum);
				Message responseObj = Message(CHAT,seqNum,chatMsg);
				q.push(responseObj);
			}
			break;
		case DELETE:
			{
				deleteUser(clientId);	
			}	
			break;
		default:
			cout<<"Invalid chat code sent by participant"<<endl;

	}
	   	
}

void Leader:: deleteUser(Id clientId) {
	if(chatRoom.find(clientId) == chatRoom.end())
		return;
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
		// receive message with ACK
		char readBuffer[500];
		bzero(readBuffer, 501);
        	Id clientId = receiveMessageWithAck(sockFd,ackSockFd,chatRoom, readBuffer);
			
		#ifdef DEBUG
        	cout<<"[DEBUG] Received from " <<clientId<<" - "<<readBuffer<<endl;
		#endif

		parseMessage(readBuffer, clientId);
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
		Message m = q.pop();
		
		// Multi-cast to everyone in the group 
		map<Id, ChatRoomUser>::iterator it;
        	for(it = chatRoom.begin(); it != chatRoom.end(); it++) {
			stringstream msgStream;
			msgStream << m.getType() << "%" << m.getMessage();
			string msg = msgStream.str();
			#ifdef DEBUG
				cout<<"[DEBUG]Sending "<<msg<<" to "<<it->second.name<<"@"<<it->first<<endl;
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
			
			if( sendMessageWithRetry(sockFd, msg.c_str(), clientAdd, ackSockFd, NUM_RETRY) == NODE_DEAD)
				deleteUser(Id(clientIp, clientPort));
		
		}		
		
		// send dequeue request to client
		vector<string> messageSplit;
		string message = m.getMessage();
		boost::split(messageSplit,message,boost::is_any_of("%"));
		Id clientId = Id(messageSplit[2]);
		struct sockaddr_in clientAdd;
                bzero((char *) &clientAdd, sizeof(clientAdd));
               	clientAdd.sin_family = AF_INET;
                inet_pton(AF_INET,clientId.ip.c_str(),&(clientAdd.sin_addr));
		clientAdd.sin_port = htons(clientId.port);
		if(sendMessageWithRetry(sockFd, to_string(DEQUEUE).c_str(), clientAdd, ackSockFd, NUM_RETRY) == NODE_DEAD)
			deleteUser(clientId);
		
	}	
}

void Leader::sendListUsers(string clientIp, int clientPort) {
	map<Id, ChatRoomUser>::iterator it;
	stringstream ss;
	ss << LIST_OF_USERS << "%" << name << "&" << ip << ":" << port <<" (Leader)"<< "&" << ackPort << "&" << heartbeatPort << "%";
	for(it = chatRoom.begin(); it != chatRoom.end(); it++) {
		ss << it->second.name << "&" << it->first << "%";		
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
		// multi-cast heartbeat
		map<Id, ChatRoomUser>::iterator it;
                for(it = chatRoom.begin(); it != chatRoom.end(); it++) {
			struct sockaddr_in clientAdd;
        		bzero((char *) &clientAdd, sizeof(clientAdd));
	        	clientAdd.sin_family = AF_INET;
			// fetch client's heartbeat details
			string clientIp = it->first.ip;
			int heartbeatPort = it->second.heartbeatPort;
        		inet_pton(AF_INET,clientIp.c_str(),&(clientAdd.sin_addr));
        		clientAdd.sin_port = htons(heartbeatPort);
			sendMessage(heartbeatSockFd, to_string(HEARTBEAT),clientAdd);	
		}		 			
		// thread sleep
		this_thread::sleep_for(chrono::milliseconds(HEARTBEAT_THRESHOLD));		
	}
}

void Leader::heartbeatReceiver() {
	struct sockaddr_in clientTemp;
	socklen_t clientTempLen = sizeof(clientTemp);
	while(true) {
		char readBuffer[500];
		bzero(readBuffer,501);
		receiveMessage(heartbeatSockFd,&clientTemp, &clientTempLen, readBuffer);
		// spliting the encoded message
	        vector<string> messageSplit;
        	boost::split(messageSplit,readBuffer,boost::is_any_of("%"));
		Id clientId = Id(messageSplit[1]);
		chatRoom[clientId].lastHbt = chrono::system_clock::now();		 		
	}
}


void Leader::detectClientFaliure(){
	while(true){
		chrono::time_point<chrono::system_clock> curTime;
		curTime = chrono::system_clock::now();
		map<Id, ChatRoomUser>::iterator it;
		for(it = chatRoom.begin(); it != chatRoom.end(); it++){
			chrono::time_point<chrono::system_clock> beatTime = it->second.lastHbt;
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
