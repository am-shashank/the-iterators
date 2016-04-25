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
	msgId = 0;
	seqNum = 0;
	name = leaderName;
	ip = getIp();
	startServer();	
	isRecovery = false;
	isNewlyElected = false;
	isRecoveryDone = false;
}
//Leader ::Leader(string tempName,string tempIp,int tempPort,int tempAckPort,int tempHeartbeatPort, struct sockaddr_in tempSock, struct sockaddr_in tempAckSock, struct sockaddr_in tempHeartbeatSock, int tempSockFd, int tempAckFd, int tempHeartbeatFd, map<Id,ChatRoomUser> tempChatRoom, string lastSeenMsg, int lastSeenSequenceNum, queue<Message> tempQ )
Leader :: Leader(string tempName, map<Id, ChatRoomUser> tempChatRoom, string lastSeenMsg, int lastSeenSequenceNum, queue<Message> tempQ)
{
	isRecoveryDone = false;
	isRecovery = false;
	isNewlyElected = true;
	msgId = 0;	
	name = tempName;
	ip = getIp();
	/*
	this->ip = tempIp;

	this->port = tempPort;
	this->ackPort = tempAckPort;
	this->heartbeatPort = tempHeartbeatPort;	
	this->sock = tempSock;
	this->ackSock = tempAckSock;
	this->heartbeatSock = tempHeartbeatSock;
	this->sockFd = tempSockFd;
	this->ackSockFd = tempAckFd;
	this->heartbeatSockFd = tempHeartbeatFd;
	*/

	// create ip:ports for urself
	
	chatRoom = tempChatRoom;

	// int minSeqNum = lastSeenSequenceNum;
	int maxSeqNum = lastSeenSequenceNum;
	int storeLastSeenSequenceNum = lastSeenSequenceNum;

	#ifdef DEBUG
	cout<<"Last Seen Seq Num" << storeLastSeenSequenceNum << endl;	
	#endif
	
	string msgToDeliver = lastSeenMsg;

	// this->sendQ = q;
	//memcpy ( &sendQ, &q, sizeof(q));

	//iterate over the queue and add it to the send q
	/*while(!tempQ.empty())
	{
		Message m = tempQ.front();
		tempQ.pop();
		this->sendQ.push(m);
	} */

	

	heartbeatSockFd = socket(AF_INET, SOCK_DGRAM, 0);
        bzero((char*) &heartbeatSock, sizeof(heartbeatSock));
        heartbeatSock.sin_family = AF_INET;
        heartbeatSock.sin_addr.s_addr = INADDR_ANY;
	heartbeatPort = generatePortNumber(heartbeatSockFd, heartbeatSock);

	// setup ACK port
	ackSockFd = socket(AF_INET, SOCK_DGRAM, 0);
        bzero((char*) &(ackSock), sizeof(ackSock));
        ackSock.sin_family = AF_INET;
        ackSock.sin_addr.s_addr = INADDR_ANY;
	ackPort = generatePortNumber(ackSockFd, ackSock);

	// create heartbeat and main socketfd
	sockFd = socket(AF_INET, SOCK_DGRAM, 0);
	bzero((char*) &sock, sizeof(sock));
    	sock.sin_family = AF_INET;
    	sock.sin_addr.s_addr = INADDR_ANY;
	port = generatePortNumber(sockFd, sock);

	cout<<"[DEBUG] I'm the new leader listening on  "<<ip<<":"<<port<<endl;
	#ifdef DEBUG
	cout<<"Socket bind successful "<<endl;
	#endif	
	isRecovery = true;
		
	// start all threads
    	// this_thread::sleep_for(chrono::milliseconds(HEARTBEAT_THRESHOLD));	
	
    	// start sender thread : TODO ensure that cin is available and sending is not happening
    	Id myId = Id(ip, port);
	ChatRoomUser meMyselfI = ChatRoomUser(name, ip, port, ackPort, heartbeatPort);
  	chatRoom[myId] = meMyselfI;	

	
	map<Id, ChatRoomUser>::iterator it2;
        for(it2 = chatRoom.begin(); it2 != chatRoom.end(); it2++) {
		Id clientId = it2->first;
		if(it2->first.ip.compare(ip) == 0 && it2->first.port == port) {
			#ifdef DEBUG
			cout<<"[DEBUG] Not sending RECOVERY"<<endl;
			#endif
			continue;			
		}
		int tempInt;
		string tempStr;
		/* set socket attributes for participant */
		struct sockaddr_in clientAdd;
		bzero((char *) &clientAdd, sizeof(clientAdd));
		clientAdd.sin_family = AF_INET;
		inet_pton(AF_INET,clientId.ip.c_str(),&(clientAdd.sin_addr));
		clientAdd.sin_port = htons(clientId.port);
		stringstream recoveryMsg;
		recoveryMsg<<RECOVERY<<"%"<<ip<<"%"<<port<<"%"<<ackPort<<"%"<<heartbeatPort;	
		sendRecoveryWithRetry(sockFd, recoveryMsg.str(), clientAdd, ackSockFd, NUM_RETRY, tempInt, tempStr);	
		// update min, max, msg
		#ifdef DEBUG
		cout << "Temp Int : " << tempInt;
		cout << "Temp Str : " << tempStr;
		#endif
		if(tempInt > maxSeqNum) {
			maxSeqNum = tempInt;
			msgToDeliver = tempStr;
		}
		/*
		if(tempInt < minSeqNum)
			minSeqNum = tempInt;
		*/
		
	}

	map<Id, ChatRoomUser>::iterator it1;
	/*#ifdef DEBUG
	cout<<"[DEBUG] Size of map : " << this->chatRoom.size();
	#endif*/
        for(it1 = this->chatRoom.begin(); it1 != this->chatRoom.end(); it1++)
		it1->second.lastHbt = chrono::system_clock::now();
	
	seqNum = maxSeqNum;
	isRecovery = false;

	
	// Start heartbeat threads
    	thread heartbeatSend(&Leader::heartbeatSender, this);
	//heartbeatSend.detach();
	thread heartbeatRecv(&Leader::heartbeatReceiver, this);
	//heartbeatRecv.detach();
    	thread detectFailure(&Leader::detectClientFaliure, this);
	//detectFailure.detach();
	thread senderThread(&Leader::sender, this);
	//senderThread.detach();


	//seqNum = maxSeqNum;	

	#ifdef DEBUG
	//cout<<"[DEBUG] max: "<<maxSeqNum<<"\tMin: "<<minSeqNum<<endl;
	#endif
	// broadcast unsynchronized message in a loop
	/*{
	//if(minSeqNum != maxSeqNum) { 
		map<Id, ChatRoomUser>::iterator it;
		for(it=chatRoom.begin(); it != chatRoom.end(); it++){
			if(it->first.ip.compare(ip) == 0 && it->first.port == port) {
				if(storeLastSeenSequenceNum != seqNum) {
					// spliting the encoded message
        				vector<string> msgSplit;
        				boost::split(msgSplit,msgToDeliver,boost::is_any_of("%"));
					int msgType = atoi(msgSplit[0].c_str());	
					if(msgType == CHAT) {
						cout<<msgSplit[1]<<endl;
					}else if(msgType == DELETE) {
						//remove that user's entry from chatRoom
		                                string key = msgSplit[1];

                		                //map<string,string> :: iterator mite;
                                		//mite = chatRoom.find(key);
                                		if(chatRoom.find(key) != chatRoom.end())
                                		{
                                        		chatRoom.erase(key);
                                		}
						cout<<msgSplit[2]<<endl;
					} else if(msgType == ADD_USER) {
						cout<<"NOTICE "<<msgSplit[1]<<" joined on "<<msgSplit[2]<<endl;
						// add the new user's information in the map
                		                Id idObj(msgSplit[2]);
		                                ChatRoomUser userObj(msgSplit[1],atoi(msgSplit[3].c_str()),atoi(msgSplit[4].c_str()));
                                		chatRoom[idObj] = userObj;
					}								
				
				}
			}else {
				string clientIp = it->first.ip;
				int clientPort = it->first.port;

				// set socket attributes for participant 
				struct sockaddr_in clientAdd;
				bzero((char *) &clientAdd, sizeof(clientAdd));
				clientAdd.sin_family = AF_INET;
				inet_pton(AF_INET,clientIp.c_str(),&(clientAdd.sin_addr));
				clientAdd.sin_port = htons(clientPort);
		
				sendMessageWithRetry(sockFd, msgToDeliver.c_str(), clientAdd, ackSockFd, NUM_RETRY);
			}
		}	
	}*/
	
//	isRecovery = false;
	// start producer, consumer threads
	#ifdef DEBUG
	cout << "Done through Recovery" << endl;
	#endif
	
	/*// Multi-cast to everyone in the group 
	map<Id, ChatRoomUser>::iterator it;
	for(it = chatRoom.begin(); it != chatRoom.end(); it++) {
		if(it->first.ip.compare(ip) == 0 && it->first.port == port)
			continue;
		stringstream msgStream;
		msgStream << m.getType() << "%" << m.getMessage() << "%" ;
		string msg = msgStream.str();
		#ifdef DEBUG
			cout<<"[DEBUG]Sending "<<msg<<" to "<<it->second.name<<"@"<<it->first<<endl;
		#endif
		string clientIp = it->first.ip;
		int clientPort = it->first.port;

		// set socket attributes for participant 
		struct sockaddr_in clientAdd;
		bzero((char *) &clientAdd, sizeof(clientAdd));
		clientAdd.sin_family = AF_INET;
		inet_pton(AF_INET,clientIp.c_str(),&(clientAdd.sin_addr));
		clientAdd.sin_port = htons(clientPort);

		sendMessageWithRetry(sockFd, msg.c_str(), clientAdd, ackSockFd, NUM_RETRY);
		
		#ifdef DEBUG
		cout<<"[DEBUG]Close recovery sent\t"<<endl;
		#endif
	}
	*/
	thread prod(&Leader::producerTask, this, sockFd);
	//prod.detach();
    	thread con(&Leader::consumerTask, this);
	//con.detach();
	
	heartbeatSend.join();
	heartbeatRecv.join();
    	detectFailure.join();
    	senderThread.join();

	con.join();
	prod.join();

	
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

    #ifdef DEBUG
    cout<<"[DEBUG] after starting server "<<endl;
    #endif
    
    printStartMessage();	
    // Start producer and consumer threads
    thread prod(&Leader::producerTask, this, sockFd);
    thread con(&Leader::consumerTask, this);	
    
    // start sender thread
    thread senderThread(&Leader::sender, this);
    	

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

    // create ACK socket
    ackSockFd = socket(AF_INET, SOCK_DGRAM, 0);
    bzero((char*) &ackSock, sizeof(ackSock));
    ackSock.sin_family = AF_INET;
    ackSock.sin_addr.s_addr = INADDR_ANY;
    ackPort = generatePortNumber(ackSockFd, ackSock);

    Id myId = Id(ip, port);

    ChatRoomUser meMyselfI = ChatRoomUser(name, ip, port, ackPort, heartbeatPort);
    chatRoom[myId] = meMyselfI;
      
    heartbeatSend.join();
    heartbeatRecv.join();
    detectFailure.join();
    senderThread.join();	
    prod.join();
    con.join();
 

}

/*  Print welcome message on start of UDP server
*/
void Leader::printStartMessage() {
	cout<<name<<" started a new chat, listening on "<<ip<<":"<<port<<endl;
	cout<<"Succeeded, current users:"<<endl;
	cout<<name<<" "<<ip<<":"<<port<<"(Leader)"<<endl;
	cout<<"Waiting for others to join..."<<endl;
		/*map<Id, ChatRoomUser>::iterator it;
		for(it = chatRoom.begin(); it != chatRoom.end(); it++) {
 		   	// iterator->first = key
			// iterator->second = value
			cout<<it->second.name<<" "<<it->first<<endl;
		}*/
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
				Message responseObj = Message(ADD_USER, response.str(), true);
				q.push(responseObj);		
			}			
			break;
		case CHAT:  	// Format: CHAT%msgId
			{
				// Check for duplicate Message
				int msgId = atoi(messageSplit[2].c_str());
				if(lastSeenMsgIdMap.find(clientId) != lastSeenMsgIdMap.end()) {
					if(msgId <= lastSeenMsgIdMap[clientId])
						break;
				}
				lastSeenMsgIdMap[clientId] = msgId;
				string senderName;
				if(clientId.ip.compare(ip) == 0 && clientId.port == port)
					senderName = name;
				else
					senderName = chatRoom[clientId].name;
				string chatMsg = senderName + ":: " + messageSplit[1] + "%"+ clientId.ip + ":" + to_string(clientId.port) + "%" + to_string(msgId);
				int curPriority = chatRoom[clientId].priority;
				curPriority++;
				Message responseObj = Message(curPriority,chatMsg, true);
				chatRoom[clientId].priority = curPriority;
				q.push(responseObj);
			
			}
			break;
		case DELETE:
			deleteUser(clientId);	
			break;
		case DEQUEUE:
			sendMessage(ackSockFd, to_string(ACK), ackSock);
			sendQ.pop();
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
	Message responseObj = Message(DELETE, response.str(), true);
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
		if(isNewlyElected) {	
			q.push(Message(CLOSE_RECOVERY,"",true));
			isNewlyElected = false;
		}		
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
	cout<<"[DEBUG]Consumer thread started. map size : " << chatRoom.size()<<endl;
	#endif	
	while(true) {
		Message m = q.pop();
		if(m.getType() == CLOSE_RECOVERY)
			isRecoveryDone = true; // allow sender to bombard messages
			
		// Multi-cast to everyone in the group 
		map<Id, ChatRoomUser>::iterator it;
		++seqNum;
        	for(it = chatRoom.begin(); it != chatRoom.end(); it++) {
			if(it->first.ip.compare(ip) == 0 && it->first.port == port)
                                continue;
			stringstream msgStream;
			msgStream << m.getType() << "%" << m.getMessage() << "%" << seqNum;
			string msg = msgStream.str();
			#ifdef DEBUG
				cout<<"[DEBUG]Sending "<<msg<<" to "<<it->second.name<<"@"<<it->first<<endl;
			#endif
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
		vector<string> msgSplit;
		string msg = m.getMessage();
                boost::split(msgSplit,msg,boost::is_any_of("%"));
		if(m.getType() >= CHAT) {
			cout<<msgSplit[0]<<endl;	
			// send dequeue request
			Id clientId = Id(msgSplit[1]);
			if(chatRoom.find(clientId) == chatRoom.end())  // Dont send dequeue messages to him
				continue;
			struct sockaddr_in clientAdd;
			bzero((char *) &clientAdd, sizeof(clientAdd));
			clientAdd.sin_family = AF_INET;
			inet_pton(AF_INET,clientId.ip.c_str(),&(clientAdd.sin_addr));
			clientAdd.sin_port = htons(clientId.port);
			if(sendMessageWithRetry(sockFd, to_string(DEQUEUE).c_str(), clientAdd, ackSockFd, NUM_RETRY) == NODE_DEAD)
				deleteUser(clientId);
						
		}else if(m.getType() == ADD_USER){
			cout<<"NOTICE "<<msgSplit[0]<<" joined on "<<msgSplit[1]<<endl;	
		}else if(m.getType() == DELETE) {
			cout<<msgSplit[1]<<endl;
		}
		
	}	
}

void Leader::sendListUsers(string clientIp, int clientPort) {
	map<Id, ChatRoomUser>::iterator it;
	stringstream ss;
	ss << LIST_OF_USERS << "%" << name << "&" << ip << ":" << port <<" (Leader)"<< "&" << ackPort << "&" << heartbeatPort << "%";
	for(it = chatRoom.begin(); it != chatRoom.end(); it++) {
		if(it->first.ip.compare(ip) == 0 && it->first.port == port)
                                continue;
		ss << it->second.name << "&" << it->first << "&" << it->second.ackPort << "&" << it->second.heartbeatPort << "%" ;		
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
	#ifdef DEBUG
	cout<<"Heart beat sender thread started"<<endl;
	#endif
	while(true){	
		// multi-cast heartbeat

		#ifdef DEBUG
		//cout<<"Heartbeat for LEADER started" << endl;
		#endif		
		
		map<Id, ChatRoomUser>::iterator it;
                for(it = chatRoom.begin(); it != chatRoom.end(); it++) {
			if(it->first.ip.compare(ip) == 0 && it->first.port == port)
                                continue;
			struct sockaddr_in clientAdd;
        		bzero((char *) &clientAdd, sizeof(clientAdd));
	        	clientAdd.sin_family = AF_INET;
			// fetch client's heartbeat details
			string clientIp = it->first.ip;
			int heartbeatPort = it->second.heartbeatPort;
        		inet_pton(AF_INET,clientIp.c_str(),&(clientAdd.sin_addr));
        		clientAdd.sin_port = htons(heartbeatPort);
			sendMessage(heartbeatSockFd, to_string(HEARTBEAT),clientAdd);	
			#ifdef DEBUG
			cout<<"[DEBUG]sending heartbeat to\t"<<heartbeatPort<<endl;
			#endif
		}		 			
		// thread sleep
		this_thread::sleep_for(chrono::milliseconds(HEARTBEAT_THRESHOLD));		
	}
}

void Leader::heartbeatReceiver() {
	#ifdef DEBUG
	cout<<"[DEBUG] Heart beat receiver thread started"<<endl;
	#endif
	struct sockaddr_in clientTemp;
	socklen_t clientTempLen = sizeof(clientTemp);
	while(true) {
		char readBuffer[500];
		bzero(readBuffer,501);
		receiveMessage(heartbeatSockFd,&clientTemp, &clientTempLen, readBuffer);
		#ifdef DEBUG
		//cout<<"[DEBUG]Heart beat received\t"<<endl;
		#endif
		// spliting the encoded message
	        vector<string> messageSplit;
        	boost::split(messageSplit,readBuffer,boost::is_any_of("%"));
		Id clientId = Id(messageSplit[1]);
		if(chatRoom.find(clientId) != chatRoom.end()) {
			chatRoom[clientId].lastHbt = chrono::system_clock::now();		
			#ifdef DEBUG
			cout<<"[DEBUG] Received heartbeat from "<<clientId<<endl;
			#endif
		}
	}
}


void Leader::detectClientFaliure(){
	#ifdef DEBUG
	cout<<"[DEBUG] Detect Client failure thread started"<<endl;
	#endif
	while(true){
		chrono::time_point<chrono::system_clock> curTime;
		curTime = chrono::system_clock::now();
		map<Id, ChatRoomUser>::iterator it;
		for(it = chatRoom.begin(); it != chatRoom.end(); it++){
			if(it->first.ip.compare(ip) == 0 && it->first.port == port)
				continue;			
			chrono::time_point<chrono::system_clock> beatTime = it->second.lastHbt;
			chrono::duration<double> elapsedSeconds = curTime - beatTime;

			// edit HEARTBEAT_THRESHOLD to required metric
			chrono::duration<double> thresholdSeconds =  chrono::duration<double>(HEARTBEAT_THRESHOLD * 0.003);

			if (elapsedSeconds.count() > thresholdSeconds.count()){
				// Node failure
				#ifdef DEBUG
				cout<<"[DEBUG] elapsed: "<<elapsedSeconds.count() <<" Threshold: "<<thresholdSeconds.count()<<endl;
				cout<<"[DEBUG] Detected node failure of "<<it->first<<endl;
				#endif
				deleteUser(it->first);
			} 
		}
		// thread sleep
		this_thread::sleep_for(std::chrono::milliseconds(HEARTBEAT_THRESHOLD));
	}
}

void Leader::sender() {
	/* sender should take input from console and send it to the leader 
        along with pushing the message in the blocking Queue*/
        char msgBuffer[500];
	
        while(true)
        {
		if(isRecoveryDone)
                {
                        // iterate over the send queue and send all the messages to the leader
                        //calculate size of the queue
                        //perform recovery
			isRecoveryDone = false;
                        //thread sendRecoveryMsgs(&Leader::performRecovery,this);
                        //sendRecoveryMsgs.detach();

                }

                bzero(msgBuffer,501);
                cin.getline(msgBuffer, sizeof(msgBuffer));
		if(cin.eof())
                {
                        // checks for Control-D
                        close(sockFd);
			close(ackSockFd);
			close(heartbeatSockFd);
        		exit(0); 
                }

		// send the message to the leader
                string msg = string(msgBuffer);
                stringstream tempStr;
                msgId = msgId+1;
                tempStr<<CHAT<<"%"<<msg<<"%"<<msgId;
                string finalMsg = tempStr.str();

		struct sockaddr_in clientAdd;
		bzero((char *) &clientAdd, sizeof(clientAdd));
		clientAdd.sin_family = AF_INET;
		inet_pton(AF_INET,ip.c_str(),&(clientAdd.sin_addr));
		clientAdd.sin_port = htons(port);
		
		Message m(msgId,msg);
                sendQ.push(m);
		
		if(!isRecoveryDone && !isRecovery) {	
			int sendResult = sendMessageWithRetry(sockFd,finalMsg,clientAdd,ackSockFd,NUM_RETRY);
			if(sendResult == -1)
			{
				#ifdef DEBUG
				cout<<"[DEBUG]message could not be sent to the leader"<<endl;
				#endif
			}
		}

	}
}

void Leader::performRecovery()
{
                // iterate over the send queue and send all the messages to the leader
                        //calculate size of the queue
                        int size = sendQ.size();
                        for(int i=0;i<size;i++)
                        {
                                Message m = sendQ.pop();
                                string msg = m.getMessage();
                                int msgId = m.getMessageId();
                                stringstream tempMsg; 
				tempMsg<<CHAT<<"%"<<msg<<"%"<<msgId;
                                string finalMsg = tempMsg.str();
				
				struct sockaddr_in clientAdd;
		                bzero((char *) &clientAdd, sizeof(clientAdd));
                		clientAdd.sin_family = AF_INET;
                		inet_pton(AF_INET,ip.c_str(),&(clientAdd.sin_addr));
                		clientAdd.sin_port = htons(port);

                                int sendResult = sendMessageWithRetry(sockFd,finalMsg, clientAdd,ackSockFd,NUM_RETRY);
                                #ifdef DEBUG
                                //cout<<"[DEBUG] Sending "<<finalMsg<<" to "<<leaderIp<<":"<<leaderPort<<endl;
                                #endif
                                if(sendResult == -1)
                                {
                                        #ifdef DEBUG
                                        cout<<"[DEBUG]message could not be sent to the leader"<<endl;
                                        #endif
                                }
                                q.push(m);
                        }
                        isRecoveryDone = false;
}


int Leader::sendRecoveryWithRetry(int sendFd, string msg, sockaddr_in addr, int ackFd, int numRetry, int &lastSeenSeqNum, string &lastSeenMsg)
{
        if(numRetry == 0)
                return NODE_DEAD;

        char writeBuffer[500];
        bzero(writeBuffer,501);
        socklen_t len = sizeof(addr);
        strncpy(writeBuffer,msg.c_str(),sizeof(writeBuffer));

        int result = sendto(sendFd,writeBuffer,strlen(writeBuffer),0,(struct sockaddr *)&addr,len);

        // wait for ACK with timeout
        struct sockaddr_in clientAdd;
        socklen_t clientLen = sizeof(clientAdd);
        char readBuffer[500];
        bzero(readBuffer, 501);
        struct timeval timeout={ TIMEOUT_RETRY, 0}; //set timeout for 2 seconds
        setsockopt(ackFd,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));
        int recvLen = recvfrom(ackFd, readBuffer, 500, 0, (struct sockaddr *) &clientAdd, &clientLen);
        // Message Receive Timeout or other error. Resend Message
        if (recvLen <= 0) {
                #ifdef DEBUG
                cout<<"[DEBUG] Retrying "<<msg<<endl;
                #endif
                sendRecoveryWithRetry(sendFd, msg, clientAdd, ackFd, numRetry - 1, lastSeenSeqNum, lastSeenMsg);
        }else {
		string recoveryMsg = string(readBuffer);
                #ifdef DEBUG
                cout<<"[DEBUG] RECOVERY ACK received "<<recoveryMsg<<endl;
                #endif
		// spliting the encoded message
	        vector<string> messageSplit;
        	boost::split(messageSplit, recoveryMsg, boost::is_any_of("$"));
		lastSeenSeqNum = atoi(messageSplit[1].c_str());
		lastSeenMsg = messageSplit[2].c_str();
        }
        return result;
}

