#include <iostream>
#include <string>
#include <cstring>
#include <boost/algorithm/string.hpp>
#include <map>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sstream>
#include <thread>
#include <poll.h>
#include <unistd.h>
#include "utils.h"
#include "globals.h"
#include <sys/time.h>
#define MIN_PORTNO 2000
#define MAX_PORTNO 50000


using namespace std;

Client :: Client(string name,string leaderIpPort)
{
	#ifdef DEBUG
	cout<<"Client Object created"<<endl;
	#endif
	userName = name;
	isLeader = false;
	msgId=0;
	iAmDead = false;
	lastSeenSequenceNum = 0;
	isOk = false;
	leaderFound = false;
	isElection = false;
	isRecovery = false;
	isRecoveryDone = false;
	multiplier=3;
	#ifdef DEBUG
	cout<<"[DEBUG] before boost"<<endl;
	#endif
	vector<string> ipPortStr;
	boost::split(ipPortStr,leaderIpPort,boost::is_any_of(":"));
	leaderIp = const_cast<char*>(ipPortStr[0].c_str());
	leaderPort = atoi(ipPortStr[1].c_str());
	#ifdef DEBUG
	cout<<"[DEBUG] after boost"<<endl;
	#endif
	establishConnection();
}

/*
function is used to set the attributes for the leader
*/
void Client :: setLeaderAttributes(char* ip, int port)
{
	leaderIp = ip;
	leaderPort = port; 
	// setup for sending CHAT/JOIN/DELETE messages to leader
	bzero((char *) &leaderAddress, sizeof(leaderAddress));
        leaderAddress.sin_family = AF_INET;
        inet_pton(AF_INET,leaderIp,&(leaderAddress.sin_addr));
        leaderAddress.sin_port = htons(leaderPort);
}
void Client:: setupLeaderPorts(int lAckPort, int heartbeatPort)
{
	//leaderAckPort = lAckPort;
        //leaderHeartBeatPort = heartbeatPort;
	
	
        // setup for sending acknowledgements to leader
        bzero((char *) &leaderAckAddress, sizeof(leaderAckAddress));
        leaderAckAddress.sin_family = AF_INET;
        inet_pton(AF_INET,leaderIp,&(leaderAckAddress.sin_addr));
        leaderAckAddress.sin_port = htons(lAckPort);

        // setup for sending heartbeat to leader
        bzero((char *) &leaderHeartBeatAddress, sizeof(leaderHeartBeatAddress));
        leaderHeartBeatAddress.sin_family = AF_INET;
        inet_pton(AF_INET,leaderIp,&(leaderHeartBeatAddress.sin_addr));
        leaderHeartBeatAddress.sin_port = htons(heartbeatPort);


}
int Client :: establishConnection()
{
	//create socket
	#ifdef DEBUG
	cout<<"[DEBUG] Inside establish connection"<<endl;
	#endif
	clientFd = socket(AF_INET,SOCK_DGRAM,0);
	int flag=1;
	
	if(clientFd < 0)
                {
                	cout<<"\nsocket could not be established\n"<<endl;
                        return 0;
                }

	#ifdef DEBUG
	//cout<<"socket created"<<endl;
	#endif	
	
	// setting the attributes for the server
	setLeaderAttributes(leaderIp,leaderPort);

	#ifdef DEBUG
	//cout<<"before setting client address"<<endl;
	#endif

	// get local ip
        clientIp = getIp();

	#ifdef DEBUG
        //cout << "[DEBUG]client ip:\t"<<clientIp<<endl;
	#endif

	bzero((char *) &clientAddress, sizeof(clientAddress));
        clientAddress.sin_family = AF_INET;
        // clientAddress.sin_addr.s_addr = inet_addr(INADDR_ANY);
	inet_pton(AF_INET,clientIp.c_str(),&(clientAddress.sin_addr));

	#ifdef DEBUG	
	//cout<<"before bind"<<endl;
	#endif

	// randomly generated port of client
	clientPort = generatePortNumber(clientFd,clientAddress);
	#ifdef DEBUG
	//cout<<"[DEBUG]port for client address structure\t"<<clientAddress.sin_port<<endl;
	#endif


	cout<<userName<<" joining a new chat on\t"<<leaderIp<<":"<<leaderPort<<", listening on "<<clientIp<<":"<<clientPort<<endl;

	// setup other client ports for sending heartbeat and receiving acknowledgements
	
	setupClientPorts();

	//send join request
	int success = joinNetwork(clientPort,clientIp);
	if(success)
	{
	/* if the client successfully joined the chat room start 3 threads for the client :- send,receive, heartbeat*/

		#ifdef DEBUG
		cout<<"[DEBUG]starting client threads"<<endl;
		#endif

		thread heartBeatPing(&Client::sendHeartbeat,this);
		thread detectFailure(&Client::detectLeaderFailure,this);
		thread sendMsg(&Client:: sender, this);
        	thread receiveMsg(&Client:: receiver, this);
		//thread processMsg(&Client:: processReceivedMessage,this);
        	sendMsg.join();
        	receiveMsg.join();
		//processMsg.join();
		detectFailure.join();
		heartBeatPing.join();
	
		close(clientFd);
		close(ackFd);
		close(heartBeatFd);
		#ifdef DEBUG
		cout<<"[DEBUG]All threads got joined inside the client"<<endl;
		#endif 

		
		if(iAmDead) {
			queue<Message> tempQ;
			int size = q.size();
			for(int i=0;i<size;i++)
			{
				Message m = q.pop();
				tempQ.push(m);

			}

			  Leader newLeader(userName, chatRoom, lastSeenMsg, lastSeenSequenceNum, tempQ);	
		}
				//close(clientFd);
		//close(heartBeatFd);
	
		//close(ackFd);
		/*
		#ifdef DEBUG
		cout<<"[DEBUG] My client closed"<<endl;
		#endif
		*/
	}
	else
	{
		// no leader in chat room
		#ifdef DEBUG
		cout<<"no leader in the chat room.. hence exiting"<<endl;
		#endif	
		exit(1);
	}	
		
}

int Client :: joinNetwork(int portNo,string ip)
{
	// calculate local ip
	#ifdef DEBUG
	cout<<"inside join network"<<endl;
	#endif	
	socklen_t len = sizeof(clientAddress);

	std::stringstream joinMsg;
	msgId=msgId+1;

	joinMsg<< JOIN<<"%"<<msgId<<"%" << userName <<"%" <<ip<<":"<<portNo<<"%"<<ackPort<<"%"<<heartBeatPort;
	string msg = joinMsg.str();

	int sendResult = sendMessageWithRetry(clientFd,msg,leaderAddress,ackFd,NUM_RETRY);
	#ifdef DEBUG
	//cout<<"[DEBUG]message sent\t"<<sendResult<<endl;
	#endif
	if(sendResult < 0)
	{
		#ifdef DEBUG
		cout<<"[DEBUG]message could not be sent"<<endl;
		#endif
	}

	
	// receiving msg
	#ifdef DEBUG
	//cout<<"[DEBUG]before receiving message"<<endl;
	#endif

	struct sockaddr_in clientTemp;
	socklen_t clientTempLen = sizeof(clientTemp);
	char readBuffer[500];
	bzero(readBuffer,501);
	#ifdef DEBUG
	//cout <<"[DEBUG]client address port\t"<<clientAddress.sin_port<<endl;
	#endif
	
	struct pollfd myPollFd;
	myPollFd.fd = clientFd;
	myPollFd.events = POLLIN;
	int result = poll(&myPollFd,1,3000);
	int receivedMessage = 0;
	if(result == -1)
	{
		#ifdef DEBUG
		cout<<"[DEBUG]error in socket timeout"<<endl;
		#endif	
	}
	else if(result == 0)
	{
		// socket timeout
		 // no chat is active
                cout<<"Sorry, no chat is active on "<<leaderIp<<":"<<leaderPort<<", try again later."<<endl<<"Bye."<<endl;
                return 0;		
	}
	else
	{
		//TODO : change receive message to receive message with ack
		receivedMessage = receiveMessage(clientFd, &clientTemp, &clientTempLen,readBuffer);
	}

	//int receivedMessage = receiveMessage(clientFd, &clientTemp, &clientTempLen,readBuffer);
		
	#ifdef DEBUG
	cout<<"[DEBUG]Message received\t"<<receivedMessage<<endl;
	//cout<<"[DEBUG] Temp socket : "<<clientTemp.sin_port<<endl;
	#endif
	if(!(receivedMessage<0))
	{
		//cout<<"Message received\t"<<receivedMessage<<endl;
		//split the message
		#ifdef DEBUG
		//cout<<"[DEBUG]Received Message\t"<<receivedMessage<<endl;
		#endif
		vector<string> listMessages ;
		string msg = string(readBuffer);
        	boost::split(listMessages,msg,boost::is_any_of("%"));
		int code = atoi(listMessages[0].c_str());
		#ifdef DEBUG
		//cout<<"[DEBUG] chat code received\t"<<code<<endl;
		//cout<<"Message received\t"<<msg<<endl;
		#endif
		if(code == LIST_OF_USERS)
		{
			// leader sent back the list of users
			cout<<"Succeeded, current users:"<<endl;
			vector<string> :: iterator ite;
			int size = listMessages.size();
			#ifdef DEBUG
			//cout<<"Size after splitting on %\t"<<size<<endl;
			#endif
			ite = listMessages.begin();
			ite = ite+1;
			int flag = 0;
			for(;ite != listMessages.end();ite++)
			{
				string element = *ite;
				#ifdef DEBUG
				//cout<<"element here\t"<<element<<endl;
				#endif
				if(!(element == ""))
				{
					vector<string> listUsers;
					listUsers.clear();
					boost::split(listUsers,element,boost::is_any_of("&"));
				
					#ifdef DEBUG
					//cout<<"size after splitting on &\t"<<listUsers.size()<<endl;
					#endif
					// users added to the map
					
                                        Id idObj(listUsers[1]);
                                        ChatRoomUser userObj(listUsers[0],atoi(listUsers[2].c_str()),atoi(listUsers[3].c_str()));
                                        chatRoom[idObj] = userObj;
					// setup ack and heartbeat ports for leader
					if(flag == 0)
					{
						// setup leader ports and send ACK to leader
						setupLeaderPorts(atoi(listUsers[2].c_str()),atoi(listUsers[3].c_str()));

						// send ack to the leader
						int sendResult = sendMessage(ackFd,to_string(ACK),leaderAckAddress);
                                		if(sendResult < 0)
                                		{
                                 		       #ifdef DEBUG
                                        		cout<<"[DEBUG]acknowledgement for join message not sent to leader"<<endl;
                                        		#endif
                                		}
 		
		
					}
					flag++;
					cout<<listUsers[0]<<"\t"<<listUsers[1]<<endl;
					//listUsers.clear();
				} 	
			}
			// all users got displayed
			return 1;
		}
		else if(code == RESOLVE_LEADER)
		{
			//some other client sent back the leader's ip and port
			vector<string> ipPort;
			boost::split(ipPort,listMessages[1],boost::is_any_of(":"));
			char *ip = const_cast<char*>(ipPort[0].c_str());
        		int portNum = atoi(ipPort[1].c_str());
			char *tempIp = const_cast<char*>(listMessages[2].c_str()); 
			int aPort = atoi(listMessages[3].c_str());

			// send acknowledgement to this client
			struct sockaddr_in tempAddr;
			bzero((char *) &tempAddr, sizeof(tempAddr));
        		tempAddr.sin_family = AF_INET;
        		inet_pton(AF_INET,tempIp,&(tempAddr.sin_addr));
        		tempAddr.sin_port = htons(aPort);
			
			int sendResult = sendMessage(ackFd,to_string(ACK),tempAddr);
                        if(sendResult < 0)
                         {
                         	#ifdef DEBUG
                               	cout<<"[DEBUG]acknowledgement for resolve leader not sent to client"<<endl;
                            	#endif
                         }

			 
			// set the attributes for the leader
			setLeaderAttributes(ip,portNum);

			//resend the join message
			joinNetwork(clientPort,clientIp);			 			
		}		
	}
	else
	{
		#ifdef DEBUG
		cout<<"[DEBUG]message could not be recived\n"<<endl;
		#endif
		// no chat is active
		cout<<"Sorry, no chat is active on "<<leaderIp<<":"<<leaderPort<<", try again later."<<endl<<"Bye."<<endl;
		return 0;
	}
				
}
 
void Client :: sender()
{
	/* sender should take input from console and send it to the leader 
	along with pushing the message in the blocking Queue*/
	char msgBuffer[500];
	// TODO : sender does not send messages if elections have started and isRecovery = true : ******DONE********
	while(true)
	{
		{
			//lock_guard<mutex> guard(iAmDeadMutex);
			if(iAmDead)
			{
				//TODO terminate the thread
				#ifdef DEBUG
				cout<<"[DEBUG]breaking from sender thread"<<endl;
				#endif
				break;
			}
		}
		
		if(isRecoveryDone)
		{
			// iterate over the send queue and send all the messages to the leader
			//calculate size of the queue
			/*perform recovery
			multiplier = 3; */
			isRecoveryDone = false;
			//thread sendRecoveryMsgs(&Client::performRecovery,this);
			//sendRecoveryMsgs.join(); */
			
		}
	
		struct pollfd fds;
		int ret;
		fds.fd = 0; // fd for stdin
		fds.events = POLLIN;
		ret = poll(&fds, 1, 0);
		bzero(msgBuffer,501);
		if(ret == 1) {		
			cin.getline(msgBuffer, sizeof(msgBuffer));	
			if(cin.eof())
			{
				// checks for Control-D
				exitChatroom();
			}	
		} else{
			//lock_guard<mutex> guard(iAmDeadMutex);		
			 if(ret == 0 && iAmDead)
				break;
		}
		
		// send the message to the leader
		
		{
			string msg = string(msgBuffer);
			#ifdef DEBUG
			cout<<"[DEBUG]mESSAGE BUFFER\t"<<msg<<endl;
			#endif
		// writing a script to send messages */
	//	string msg="";
	//	for(int i=0;i<5;i++)
		
	//		if(i==6)
	//		{break;}
	//		msg = to_string(i);
		
		
		if(msg.compare("")!=0 && !iAmDead)
		{
			
			#ifdef DEBUG
			//cout<<"[DEBUG] Inside sender thread: "<<msg<<endl;
			#endif	
			stringstream tempStr;
			msgId = msgId+1;
			tempStr<<CHAT<<"%"<< msg<<"%"<<msgId;
			string finalMsg = tempStr.str();
			#ifdef DEBUG
			cout<<"Message sent\t"<<finalMsg<<endl;
			#endif
			//int sendResult = sendMessage(clientFd,finalMsg,leaderAddress);
		
			// create an object of the message class and enque the message
		       
			Message m(msgId,msg);
			//q.push(m);
			
			// send message to the leader only if no elections are being held and new leader is not recovering information
			//lock_guard<mutex> guard(isElectionMutex);
			if(!isElection && !isRecovery && !isRecoveryDone)
			{
				q.push(m);
				int sendResult = sendMessageWithRetry(clientFd,finalMsg,leaderAddress,ackFd,NUM_RETRY);
				#ifdef DEBUG
				//cout<<"[DEBUG] Sending "<<finalMsg<<" to "<<leaderIp<<":"<<leaderPort<<endl;
				#endif
				if(sendResult == -1)
				{
					#ifdef DEBUG
					cout<<"[DEBUG]message could not be sent to the leader"<<endl;
					#endif
				}
				else if(sendResult == NODE_DEAD)
				{
					//leader died and hence perform elections
					//TODO: perform elections : *******DONE********
					if(!isElection)
					{
								
						thread electionThread(&Client::startElection,this);
						//electionThread.join();
						electionThread.detach();
					}
				}
			}
		}		
	}
		break;		
	}
	// senderSem.notify();
}
void Client ::performRecovery()
{
		// iterate over the send queue and send all the messages to the leader
                        //calculate size of the queue
                        int size = q.size();
                        for(int i=0;i<size;i++)
                        {
                                Message m = q.pop();
                                string msg = m.getMessage();
                                int msgId = m.getMessageId();
                                stringstream tempMsg; tempMsg<<CHAT<<"%"<<msg<<"%"<<msgId;
                                string finalMsg = tempMsg.str();
                                int sendResult = sendMessageWithRetry(clientFd,finalMsg,leaderAddress,ackFd,NUM_RETRY);
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
void Client :: receiver()
{
	struct sockaddr_in clientTemp;
	socklen_t clientTempLen = sizeof(clientTemp);

	/* receiver thread should wait to receive the message from leader or from 
	other clients, verify and dequeue it from the blocking queue*/


	socklen_t len = sizeof(clientAddress);
	while(true)
	{
		//struct sockaddr_in clientTemp;
	        //socklen_t clientTempLen = sizeof(clientTemp);
		//socklen_t len = sizeof(clientAddress);
	
		char readBuffer[500];
		bzero(readBuffer,501);
	
		/*struct pollfd myPollFd;
		myPollFd.fd = heartBeatFd;
		myPollFd.events = POLLIN;
		int result = poll(&myPollFd,1,0);
		int receivedMessage = 0;
		if(result == -1)
		{
			#ifdef DEBUG
			cout<<"[DEBUG]error in socket timeout"<<endl;
			#endif
		}
		else if(result == 0)
		{
			if(iAmDead)
				break;
		}
		else*/
		{
			if(iAmDead)
				break;	
			int numChar = receiveMessage(clientFd,&clientTemp,&clientTempLen,readBuffer);
			
			if(numChar<=0)
			{
				#ifdef DEBUG
				cout<<"[DEBUG]No message received"<<endl;
				#endif
			}
			else
			{
				#ifdef DEBUG
				cout<<"[DEBUG] Recevied message "<<readBuffer<<endl;
				#endif
				string tempBuffer = string(readBuffer);
				string msg;
				vector<string> msgSplit;
				vector<string> :: iterator ite;
				bool isCloseR = false;

				//check if the prefix is CLOSE_RECOVERi(msg.at(0) == CLOSE_RECOVERY)
				if(tempBuffer.at(0) == '0')
					{
						// CASE for CLOSE_RECOVERY
						
						isCloseR = true;
						vector<string> tempSplit;
						boost::split(tempSplit,tempBuffer,boost::is_any_of("$"));
						msg = tempSplit[1];								
					}
				else
				{
					msg = tempBuffer;
				}
				// splitting the message on the basis of %
				//vector<string> msgSplit;
				//vector<string> :: iterator ite;
				boost::split(msgSplit,msg,boost::is_any_of("%"));
				ite = msgSplit.begin();
				ite++;
				int code = atoi(msgSplit[0].c_str());

				if(isCloseR)
				{

					// main code for CLOSE_RECOVERY
					// TODO set isRecovery = false: ****DONE****
                                        multiplier = 3;
                                        isRecovery = false;
                                        isRecoveryDone = true;
                                        isElection = false;
                                        // send acknowledgement to the user
                                        sendAck(to_string(ACK));
	
				}
				if(code == IAMDEAD)
				{
					// terminate thread
					// lock_guard<mutex> guard(iAmDeadMutex);
					iAmDead = true;
					//send acknowledgement
					sendMessage(ackFd,to_string(ACK),ackAddress);			
					#ifdef DEBUG
					cout<<"[DEBUG]breaking out of receiver thread"<<endl;
					#endif
					// close(clientFd);
					// close(heartBeatFd);
					break;
				
				}
				
				else if(code >= CHAT)
				{
					/*
					for(;ite!= msgSplit.end();ite++)
					{
						cout<<*ite<<"\t";
					}	
					*/
					stringstream msg;
					msg<<ACK;
					string ackMsg = msg.str();
					
					// send acknowledgements
					sendAck(ackMsg);

					//lastSeenMsg = msgSplit[1];
					string ipPort = msgSplit[2];
					int mid = atoi(msgSplit[3].c_str());
					int currentSequenceNum = atoi(msgSplit[4].c_str());
					
					if(currentSequenceNum >  lastSeenSequenceNum)
					{
						lastSeenMsg = string(readBuffer);
						lastSeenSequenceNum = currentSequenceNum;
						cout<<msgSplit[1]<<endl;		
					}
				}
				else if(code == JOIN)
				{
					// if the client receives a join request
					// send the leader's Ip and port to the respective client
								
					vector<string> ipPort;
					boost::split(ipPort,msgSplit[3],boost::is_any_of(":"));
					char *ip = const_cast<char*>(ipPort[0].c_str());
					int aPort = atoi(msgSplit[4].c_str());

					// send acknowledgement to this client
					struct sockaddr_in tempAddr;
					bzero((char *) &tempAddr, sizeof(tempAddr));
					tempAddr.sin_family = AF_INET;
					inet_pton(AF_INET,ip,&(tempAddr.sin_addr));
					tempAddr.sin_port = htons(aPort);

					int sendResult = sendMessage(ackFd,to_string(ACK),tempAddr);
					if(sendResult < 0)
					{
						#ifdef DEBUG
						cout<<"[DEBUG]acknowledgement for join message not sent to client"<<endl;
						#endif
					}

					
					
					if(!isLeader)
					{

						// first send ACK to that client
						// spilt/parse message
						

						stringstream resolveMsg;
						resolveMsg<<RESOLVE_LEADER<<"%"<<leaderIp<<":"<<leaderPort<<"%"<<clientIp<<"%"<<ackPort;
						string msg = resolveMsg.str();
						//send the leaderIp & leaderPort to client
						int sendResult = sendMessage(clientFd,msg,clientTemp);
						if(sendResult < 0)
						{
							#ifdef DEBUG
							cout<<"[DEBUG]message could not be sent to the client"<<endl;
							#endif
						}					
					}
									
				}
				else if(code == DELETE)
				{

					//send acknowledgement first
					sendAck(to_string(ACK));

				

					//remove that user's entry from chatRoom
					string key = msgSplit[1];
						
					//map<string,string> :: iterator mite;
					//mite = chatRoom.find(key);
					if(chatRoom.find(key) != chatRoom.end())
					{
						chatRoom.erase(key);
					}
					#ifdef DEBUG
					cout <<"[DEBUG]user deleted from the map"<<endl;
					#endif
					int currentSequenceNum = atoi(msgSplit[3].c_str());

					if(currentSequenceNum >  lastSeenSequenceNum)
					{
						lastSeenMsg = string(readBuffer);
						lastSeenSequenceNum = currentSequenceNum;
						cout<<msgSplit[2]<<endl;		
					}
				}
				else if(code == DEQUEUE)
				{
					// dequeue the top message from the queue
					//send acknowledgement first
					sendAck(to_string(ACK));
					//dequeue message from the front of queue
					q.pop();	
					
				}
				else if(code == ELECTION)
				{
					// TODO : send ack to the sender : ******DONE*********
					
					#ifdef DEBUG
					cout<<"[DEBUG]election message received"<<endl;
					#endif
					Id idObj = getId(clientTemp);
					int aPort = chatRoom[idObj].ackPort;

					// create the sockaddr for that client
					struct sockaddr_in tempAddr;
					bzero((char *) &tempAddr, sizeof(tempAddr));
					tempAddr.sin_family = AF_INET;
					inet_pton(AF_INET,idObj.ip.c_str(),&(tempAddr.sin_addr));
					tempAddr.sin_port = htons(aPort);
					int sendResult = sendMessage(ackFd,to_string(ACK),tempAddr);
					#ifdef DEBUG
					cout<<"[DEBUG]ACK sent for election message "<<endl;
					#endif
					
					// TODO : set election if election has not been called : *******DONE*********
					// lock_guard<mutex> guard(isElectionMutex);
					if(!isElection)
					{
						// if election did not start yet, start elections
						
						thread electionThread(&Client::startElection,this);
						electionThread.detach();
						//electionThread.join();
					}
					
				}
				else if(code == LEADER)
				{
					multiplier *= 3;
					#ifdef DEBUG
					cout<<"[DEBUG]received leader message from the new leader"<<endl;
					#endif
					// leader is found
					// lock_guard<mutex> guard(leaderFoundMutex);
					leaderFound = true;
					//set election to false
					//isElection = false;

					// send the acknowledgement
					Id idObj = getId(clientTemp);
					int aPort = chatRoom[idObj].ackPort;
					// create the sockaddr for that client
					struct sockaddr_in tempAddr;
					bzero((char *) &tempAddr, sizeof(tempAddr));
					tempAddr.sin_family = AF_INET;
					inet_pton(AF_INET,idObj.ip.c_str(),&(tempAddr.sin_addr));
					tempAddr.sin_port = htons(aPort);
					int sendResult = sendMessage(ackFd,to_string(ACK),tempAddr);
					if(sendResult < 0)
					{
						#ifdef DEBUG
						cout<<"[DEBUG]failed to send ACK to the new leader"<<endl;
						#endif
					}
					// TODO set all leader attributes
					// invoking functions to set all leader attributes
					//char *tempIp =const_cast<char*>(idObj.ip.c_str());
					//setLeaderAttributes(tempIp,idObj.port);
					//setupLeaderPorts(chatRoom[idObj].ackPort,chatRoom[idObj].heartbeatPort);
					#ifdef DEBUG
					//cout<<"[DEBUG]IP..PORT..ACKPORT..HEARTpORT...\t"<<tempIp<<" "<<idObj.port<<"  "<<chatRoom[idObj].ackPort<<" "<<chatRoom[idObj].heartbeatPort<<endl;
					#endif
					// lock_guard<mutex> guard1(isElectionMutex);
					isElection = false;					
				}
				else if(code == RECOVERY)
				{
					//TODO : set isElection as false : ******DONE******
					// TODO : isRecovery on : *****DONE*****
					//TODO  send ACK+
					//TODO send lastSeenSequenceNumber and lastSeenMsg on ack port : *****DONE*****
					
					
					// reset boolean flags

					isRecovery = true;
					multiplier *= 2;
					isElection = false;
					#ifdef DEBUG
					cout<<"[DEBUG]GOT RECOVERY MSG"<<endl;
					#endif

					//set attributes for the new leader here
					char *tempIp =const_cast<char*>(msgSplit[1].c_str());
					setLeaderAttributes(tempIp,atoi(msgSplit[2].c_str()));
                                        setupLeaderPorts(atoi(msgSplit[3].c_str()),atoi(msgSplit[4].c_str()));

					// send acknowledgement for recovery message to new leader
                                        stringstream msg;
                                        msg<<ACK<<"$"<<lastSeenSequenceNum<<"$"<<lastSeenMsg;
                                        sendAck(msg.str());
					
				}
				/*else if(code == CLOSE_RECOVERY)
				{
					// TODO set isRecovery = false: ****DONE****
					multiplier = 3;
					isRecovery = false;
					isRecoveryDone = true;
					isElection = false;
					// send acknowledgement to the user
					sendAck(to_string(ACK));

				}*/		
				else if(code == ADD_USER)
				{
					// send acknowledgement to the leader
					sendAck(to_string(ACK));
					int currentSequenceNum = atoi(msgSplit[5].c_str());

					if(currentSequenceNum >  lastSeenSequenceNum)
					{
						lastSeenMsg = string(readBuffer);
						lastSeenSequenceNum = currentSequenceNum;
						// print the message on the screen
						cout<<"NOTICE "<<msgSplit[1]<<" joined on "<<msgSplit[2]<<endl;
		
					}
					// add the new user's information in the map
					Id idObj(msgSplit[2]);
					ChatRoomUser userObj(msgSplit[1],atoi(msgSplit[3].c_str()),atoi(msgSplit[4].c_str()));
					chatRoom[idObj] = userObj;		

					#ifdef DEBUG
					cout<<"[DEBUG]New user added in the chat room map"<<endl;
					#endif
				}
				cout<<endl;
				 
			}

				
		}
	}
	// receiverSem.notify();
}
void Client :: sendAck(string msg)
{
	int sendResult = sendMessage(ackFd,msg,leaderAckAddress);
	Id obj = getId(leaderAckAddress);
	
	#ifdef DEBUG
	cout<<"ACK sent for "<<msg<<"to\t"<<obj.ip<<":"<<obj.port<<endl;
	#endif
        if(sendResult < 0)
        {
            #ifdef DEBUG
            cout<<"[DEBUG]acknowledgement could not be sent"<<endl;
            #endif
        }
	
}

// start elections
void Client:: startElection()
{
	#ifdef DEBUG
	cout<<"[DEBUG]elections started"<<endl;
	#endif
	// lock_guard<mutex> guard4(isElectionMutex);	
	isElection = true;
	Id idObj(leaderIp,leaderPort);
	chatRoom.erase(idObj);
	
	do
	{
		isOk = false;
		map<Id, ChatRoomUser>::iterator it;
                for(it = chatRoom.begin(); it != chatRoom.end(); it++)
		{
			if(clientPort < it->first.port)
			{
				#ifdef DEBUG
				cout<<"sending election message to higher ports"<<endl;
				#endif

				string ip = it->first.ip;
				int port = it->first.port;

				// create the ackFd and sockaddr for that client
				struct sockaddr_in tempAddr;
                                bzero((char *) &tempAddr, sizeof(tempAddr));
                                tempAddr.sin_family = AF_INET;
                                inet_pton(AF_INET,ip.c_str(),&(tempAddr.sin_addr));
                                tempAddr.sin_port = htons(port);
				//send election message to the user
				int sendResult = sendMessageWithRetry(clientFd,to_string(ELECTION),tempAddr,ackFd,NUM_RETRY);
							
				if(sendResult != NODE_DEAD && sendResult>=0)
				{
		
					isOk = true;
					#ifdef DEBUG
                                	cout<<"[DEBUG]ACK received for election message"<<endl;
                                	#endif
				}
		
			}
		}
		// sleep Ifor sometime to detect leader or receive OK message
		this_thread::sleep_for(chrono::milliseconds(3000));

		//check if the leader was found or not		
		// lock_guard<mutex> guard3(leaderFoundMutex);
		if(leaderFound)
		{
			// found leader so stop election thread
			//TODO : means someone else is leader/ set new leader attribs : *****DONE*****
			//TODO : do same thing as been done in CASE : LEADER
			// already this work is done so just break because leaderFound will be true if and only if CASE: LEADER is reached
			break;
		}


	}while(isOk);
	
	// lock_guard<mutex> guard2(leaderFoundMutex);
	if(!leaderFound)
	{
		// TODO leader has not been found so declare itself as leader and stop elections: ******DONE******
	
		// send broadcast messages to everyone in the map
		#ifdef DEBUG
		cout<<"I am leader and my port number is\t"<<clientPort<<endl;
		#endif			
		map<Id, ChatRoomUser>::iterator it;
                for(it = chatRoom.begin(); it != chatRoom.end(); it++)
                {
				if((it->first.ip.compare(clientIp)==0) && (it->first.port == clientPort) )
					continue;
                		string ip = it->first.ip;
                        	int port = it->first.port;

                        	// create the ackFd and sockaddr for that client
                        	struct sockaddr_in tempAddr;
                                bzero((char *) &tempAddr, sizeof(tempAddr));
                                tempAddr.sin_family = AF_INET;
                                inet_pton(AF_INET,ip.c_str(),&(tempAddr.sin_addr));
                                tempAddr.sin_port = htons(port);
                                //send election message to the user
                                int sendResult = sendMessageWithRetry(clientFd,to_string(LEADER),tempAddr,ackFd,NUM_RETRY);
                                if(sendResult == NODE_DEAD)
				{
					// if the node is dead then remove that node from the map 
					chatRoom.erase(it->first);
				}

		}

		//isElection = false;
		#ifdef DEBUG
		cout<<"[DEBUG]leader message broadcasted to all other clients"<<endl;
		#endif

		// TODO create leader object: *****DONE*****	
		//TODO create new leader constructor with name,ip,ports and chatroom map without your own : ******DONE*******
	
		//TODO close all client threads/sockets  and detach

		// remove this new leader from the map
		Id obj(clientIp,clientPort);
		ChatRoomUser clientUser = chatRoom[obj];
		chatRoom.erase(obj);
		
		// TODO take care of all the previous client threads
		
		// creating a new leader object: *****DONE******
		/*queue<Message> tempQ;	
		int size = q.size();
		for(int i=0;i<size;i++)
		{
			Message m = q.pop();
			tempQ.push(m);
				
		}*/
		iAmDead = true;
		struct sockaddr_in clienttemp;
	        bzero((char *) &clienttemp, sizeof(clienttemp));
        	clienttemp.sin_family = AF_INET;
        	inet_pton(AF_INET,clientIp.c_str(),&(clienttemp.sin_addr));
        	clienttemp.sin_port = htons(clientPort);
		
		sendMessageWithRetry(clientFd, to_string(IAMDEAD), clienttemp, ackFd,NUM_RETRY);	// unblock the receive
		
		/*senderSem.wait();
		receiverSem.wait();
		sendHeartBeatSem.wait();
		detectLeaderFailureSem.wait();
		*/
		/*Leader newLeader(clientUser.name,clientIp,clientPort,clientUser.ackPort,clientUser.heartbeatPort, clientAddress, heartBeatAddress, ackAddress, clientFd, ackFd, heartBeatFd, chatRoom, lastSeenMsg, lastSeenSequenceNum, tempQ);
*/				
	}
	#ifdef DEBUG
        cout<<"[DEBUG]exiting election thread"<<endl;
        #endif
}


void Client :: exitChatroom() 
{
	#ifdef DEBUG
	cout<<"[DEBUG]"<<userName<< " exiting...."<<endl;
	#endif	
	stringstream deleteRequest;
	deleteRequest<<DELETE<<"%"<<clientIp<<":"<<clientPort;

	string deleteMsg = deleteRequest.str(); 	
	int sendResult = sendMessage(clientFd,deleteMsg,leaderAddress);
	if(sendResult < 0)
	{
		#ifdef DEBUG
		cout<<"[DEBUG]delete request could not be sent\t"<<endl;
		#endif
	}
	
	close(clientFd);
	close(ackFd);
	close(heartBeatFd);
	exit(0);
}

// setup heartbeat socket and acknowledgement sockets
void Client :: setupClientPorts()
{
	heartBeatFd = socket(AF_INET,SOCK_DGRAM,0);
			
	if(heartBeatFd < 0)
	{
		#ifdef DEBUG
		cout<<"[DEBUG]socket creation for heart beat failed"<<endl;
		#endif
	}
	
	// set timeout for the socket

	
	bzero((char *) &heartBeatAddress, sizeof(heartBeatAddress));
        heartBeatAddress.sin_family = AF_INET;
        inet_pton(AF_INET,clientIp.c_str(),&(heartBeatAddress.sin_addr));

        // randomly generated port of client for heartbeat
        heartBeatPort = generatePortNumber(heartBeatFd,heartBeatAddress);

	/*struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 15*HEARTBEAT_THRESHOLD;
        
        if(setsockopt(heartBeatFd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout))<0)
        {
                #ifdef DEBUG
                cout<<"[DEBUG]error occurred while executing setsockopt() for heart beat socket"<<endl;
                #endif
        }*/
	
	// setting port for receiving acknowledgements
	ackFd = socket(AF_INET,SOCK_DGRAM,0);
	if(ackFd < 0)
	{
		#ifdef DEBUG
		cout<<"[DEBUG]socket creation for acknowledgements failed\t"<<endl;
		#endif		
	}
	bzero((char *) &ackAddress, sizeof(ackAddress));
        ackAddress.sin_family = AF_INET;
        inet_pton(AF_INET,clientIp.c_str(),&(ackAddress.sin_addr));
        // randomly generated port of client for acknowledgements
        ackPort = generatePortNumber(ackFd,ackAddress);
	 	
}

// TODO : if elections,don't send the heart beat since no leader now : ******DONE******
void Client :: sendHeartbeat()
{
	//the function should periodically send a heartbeat ping to the leader
	stringstream tempStr;
        tempStr<<HEARTBEAT<<"%"<<clientIp<<":"<<clientPort;
        string finalMsg = tempStr.str();
	// send heart beats only if the elections are not being held
	while(true)
	{
		// lock_guard<mutex> guard7(iAmDeadMutex);
		if(iAmDead)
		{
			#ifdef DEBUG
			cout<<"[DEBUG]breaking out of heartbeat thread"<<endl;
			#endif
			break;
		}
		// lock_guard<mutex> guard6(isElectionMutex);
		if(!isElection)
		{
			#ifdef DEBUG
			//cout<<"[DEBUG] sending heartbeats...no elections "<<getId(leaderHeartBeatAddress)<<endl;
			#endif
        		int sendResult = sendMessage(heartBeatFd,finalMsg,leaderHeartBeatAddress);
			if(sendResult == -1)
			{
				#ifdef DEBUG
				//cout<<"[DEBUG]failed to send heartbeat to the leader"<<endl;
				#endif
			
			}
			this_thread::sleep_for(chrono::milliseconds(HEARTBEAT_THRESHOLD));
		}
		else
		{
			#ifdef DEBUG
			//cout<<"[DEBUG]elections started...no heartbeat"<<endl;
			#endif
		}
	}
	// sendHeartBeatSem.notify();
}
// TODO : if elections terminate detectLeaderFailure : ******DONE******
void Client :: detectLeaderFailure()
{

	struct sockaddr_in clientTemp;
        socklen_t clientTempLen = sizeof(clientTemp);
	// receive the heart beats and detect for failure of the leader only if no elections are being held
	while(true)
	{
		// lock_guard<mutex> guard9(iAmDeadMutex);

		if(iAmDead)
		{
			#ifdef DEBUG
                        cout<<"[DEBUG]breaking out of receive heartbeat thread"<<endl;
                        #endif
			break;
		}
		// lock_guard<mutex> guard8(isElectionMutex);
		if(!isElection)
		{
			/*
			#ifdef DEBUG
			cout<<"[DEBUG]NOT IS ELECTION"<<endl;
			#endif
			*/
			struct pollfd myPollFd;
			myPollFd.fd = heartBeatFd;
			myPollFd.events = POLLIN;
			int result = poll(&myPollFd,1,multiplier*HEARTBEAT_THRESHOLD);

			int receivedMessage = 0;
			if(result == -1)
			{
				#ifdef DEBUG
				cout<<"[DEBUG]error in socket timeout"<<endl;
				#endif
			}
			else if(result == 0)
			{
				// socket timeout
				#ifdef DEBUG
                        	cout<<"[DEBUG]no heart beat received from the leader... leader failed"<<endl;
                        	#endif
                        	// TODO : start elections since the leader failed: *****DONE******
				if(!isElection)
				{
					
					thread electionThread(&Client::startElection,this);
					//electionThread.join();
					electionThread.detach();
				} 
			}
			else if(!iAmDead)
			{
				char readBuffer[500];
	                	bzero(readBuffer,501);
        	        	int numChar = receiveMessage(heartBeatFd,&clientTemp,&clientTempLen,readBuffer);
				#ifdef DEBUG
				//cout<<"[DEBUG]heartbeat received from leader"<<endl;
				#endif
			}
		}

	}			
	// detectLeaderFailureSem.notify();	
}
