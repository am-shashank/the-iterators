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
#include "utils.h"
#include "globals.h"
#include <sys/time.h>
#define MIN_PORTNO 2000
#define MAX_PORTNO 50000


using namespace std;

Client :: Client(string name,string leaderIpPort)
{
	userName = name;
	isLeader = false;
	msgId=0;
	vector<string> ipPortStr;
	boost::split(ipPortStr,leaderIpPort,boost::is_any_of(":"));
	leaderIp = const_cast<char*>(ipPortStr[0].c_str());
	leaderPort = atoi(ipPortStr[1].c_str());
	establishConnection();
}

/*
function is used to set the attributes for the server
*/
void Client :: setLeaderAttributes(char* ip, int port)
{
	leaderIp = ip;
	leaderPort = port;
	bzero((char *) &leaderAddress, sizeof(leaderAddress));
        leaderAddress.sin_family = AF_INET;
        inet_pton(AF_INET,leaderIp,&(leaderAddress.sin_addr));
        leaderAddress.sin_port = htons(leaderPort);
		
}
int Client :: establishConnection()
{
	//create socket

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
	/*
	struct timeval t1;
        gettimeofday(&t1, NULL);
        srand(t1.tv_usec * t1.tv_sec);
	
	while(true) {

                int range = MAX_PORTNO - MIN_PORTNO + 1;
                clientPort = rand() % range + MIN_PORTNO;
		#ifdef DEBUG
		//cout<<"[DEBUG]client port generated\t"<<clientPort<<endl;
		#endif
                clientAddress.sin_port = htons(clientPort);
		#ifdef DEBUG
		//cout <<"[DEBUG] sin_port generated\t"<<htons(clientPort)<<endl;
		#endif

                if(bind(clientFd, (struct sockaddr *)&clientAddress, sizeof(clientAddress)) < 0) {
                        cerr << "Error: Cannot bind socket on " <<clientPort<<endl;
                }else
                        break;

        }
	*/
	#ifdef DEBUG
	//cout<<"[DEBUG]port for client address structure\t"<<clientAddress.sin_port<<endl;
	#endif


        //clientAddress.sin_port = htons(clientPort);
	// set timeout for client socket
	/*struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 5000;
	
	if(setsockopt(clientFd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout))<0)
	{
		#ifdef DEBUG
		cout<<"[DEBUG]error occurred while executing setsockopt()"<<endl;
		#endif
	}*/

	//since bind was successful print the message on client's screen
	cout<<userName<<" joining a new chat on\t"<<leaderIp<<":"<<leaderPort<<", listening on "<<clientIp<<":"<<clientPort<<endl;

	int success = joinNetwork(clientPort,clientIp);
	if(success)
	{
	/* if the client successfully joined the chat room start 3 threads for the client :- send,receive, heartbeat*/

		#ifdef DEBUG
		cout<<"[DEBUG]starting client threads"<<endl;
		#endif

		thread sendMsg(&Client:: sender, this);
        	thread receiveMsg(&Client:: receiver, this);
        	sendMsg.join();
        	receiveMsg.join();
 
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
	
	socklen_t len = sizeof(clientAddress);

	std::stringstream joinMsg;

	joinMsg<< JOIN<<"%" << userName <<"%" <<ip<<":"<<portNo;
	string msg = joinMsg.str();

	int sendResult = sendMessage(clientFd,msg,leaderAddress);
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
					if(flag != 0)
					{
						chatRoom[listUsers[1]] = listUsers[0];
					}
					flag++;
					cout<<listUsers[0]<<"\t"<<listUsers[1]<<endl;
					listUsers.clear();
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
	
	while(true)
	{
		
		
		bzero(msgBuffer,501);
		cin.getline(msgBuffer, sizeof(msgBuffer));	
		if(cin.eof())
		{
			// checks for Control-D
			exitChatroom();
		}	
		
		// send the message to the leader
		string msg = string(msgBuffer);
		stringstream tempStr;
		tempStr<<CHAT<<"%"<<msg;
		string finalMsg = tempStr.str();
		int sendResult = sendMessage(clientFd,finalMsg,leaderAddress);
		#ifdef DEBUG
		//cout<<"[DEBUG] Sending "<<finalMsg<<" to "<<leaderIp<<":"<<leaderPort<<endl;
		#endif
		if(sendResult == -1)
	        {
        	        #ifdef DEBUG
                	cout<<"[DEBUG]message could not be sent to the leader"<<endl;
                	#endif
        	}
		// enque the message in the clientQueue
		
		// create an object of the message class
		msgId = msgId+1;
		Message m(msgId,msg);
		q.push(m);		
		
	}
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
		char readBuffer[500];
		bzero(readBuffer,501);
		
			
		#ifdef DEBUG
		//cout<<"[DEBUG] Before Recieve message in receiver"<<endl;
		#endif
		int numChar = receiveMessage(clientFd,&clientTemp,&clientTempLen,readBuffer);
		if(numChar<0)
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
			string msg = string(readBuffer);
			// splitting the message on the basis of %
			vector<string> msgSplit;
			vector<string> :: iterator ite;
			boost::split(msgSplit,msg,boost::is_any_of("%"));
			ite = msgSplit.begin();
			ite++;
			int code = atoi(msgSplit[0].c_str());
			if(code == CHAT)
			{
				for(;ite!= msgSplit.end();ite++)
				{
					cout<<*ite<<"\t";
				}	
			}
			else if(code == JOIN)
			{
				// if the client receives a join request
				// send the leader's Ip and port to the respective client
				if(!isLeader)
				{
					stringstream resolveMsg;
					resolveMsg<<RESOLVE_LEADER<<"%"<<leaderIp<<":"<<leaderPort;
					string msg = resolveMsg.str();
					//send the leaderIp & leaderPort to client

					/*
					vector<string> ipPort;
					boost::split(ipPort,msgSplit[1],boost::is_any_of(":"));
					char *ip = const_cast<char*>(ipPort[0].c_str());
                        		int portNum = atoi(ipPort[1].c_str());
						
					struct sockaddr_in tempClient;
					bzero((char *) &tempClient, sizeof(tempClient));
        				tempClient.sin_family = AF_INET;
        				inet_pton(AF_INET,ip,&(tempClient.sin_addr));
        				tempClient.sin_port = htons(portNum);
					
					*/
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
				// if the chat code sent is DELETE then 
				//remove that user's entry from chatRoom
				string key = msgSplit[1];
				//map<string,string> :: iterator mite;
				//mite = chatRoom.find(key);
				chatRoom.erase(key);
				#ifdef DEBUG
				cout <<"[DEBUG]user deleted from the map"<<endl;
				#endif
				cout<<msgSplit[2]<<endl;		
			}
			cout<<endl;
			 
		}

		//TODO: verify the message and then accordingly dequeue it from the queue	
	}
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
	// TODO: close socket, termine thread	
	close(clientFd);
	exit(0);

}
