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
#include "utils.h"
#include "globals.h"

#define MIN_PORTNO 2000
#define MAX_PORTNO 50000


using namespace std;

Client :: Client(string name,string leaderIpPort)
{
	userName = name;
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
	cout<<"socket created"<<endl;
	#endif	
	
	// setting the attributes for the server
	setLeaderAttributes(leaderIp,leaderPort);

	#ifdef DEBUG
	cout<<"before setting client address"<<endl;
	#endif

	// get local ip
        clientIp = getIp();

	#ifdef DEBUG
        cout << "client ip:\t"<<clientIp<<endl;
	#endif

	bzero((char *) &clientAddress, sizeof(clientAddress));
        clientAddress.sin_family = AF_INET;
        //clientAddress.sin_addr.s_addr = inet_addr(INADDR_ANY);
	inet_pton(AF_INET,clientIp.c_str(),&(clientAddress.sin_addr));

	#ifdef DEBUG	
	cout<<"before bind"<<endl;
	#endif

	// randomly generated port of client
	while(true) {

                int range = MAX_PORTNO - MIN_PORTNO + 1;
                clientPort = rand() % range + MIN_PORTNO;
                clientAddress.sin_port = htons(clientPort);

                if(bind(clientFd, (struct sockaddr *)&clientAddress, sizeof(clientAddress)) < 0) {
                        cerr << "Error: Cannot bind socket on " <<clientPort<<endl;
                }else
                        break;

        }
	//cout<<"after bind"<<endl;

        clientAddress.sin_port = htons(clientPort);

	//since bind was successful print the message on client's screen
	cout<<userName<<" joining a new chat on\t"<<leaderIp<<":"<<leaderPort<<", listening on "<<clientIp<<":"<<clientPort<<endl;

	int success = joinNetwork(clientPort,clientIp);
	if(success)
	{
	/* if the client successfully joined the chat room start 3 threads for the client :- send,receive, heartbeat*/

		#ifdef DEBUG
		cout<<"starting client threads"<<endl;
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

	if(sendResult == -1)
	{
		#ifdef DEBUG
		cout<<"[DEBUG]message could not be sent"<<endl;
		#endif
	}

	
	// receiving msg

	string receivedMessage = receiveMessage(clientFd,&clientAddress,&len);
	if(!(receivedMessage == ""))
	{
		//cout<<"Message received\t"<<receivedMessage<<endl;
		//split the message
		vector<string> listMessages ;
        	boost::split(listMessages,receivedMessage,boost::is_any_of("%"));
		int code = atoi(listMessages[0].c_str());
		if(code == LIST_OF_USERS)
		{
			// leader sent back the list of users
			cout<<"Succeeded, current users:"<<endl;
			vector<string> :: iterator ite;
			ite = listMessages.begin();
			ite = ite+2;
			for(;ite != listMessages.end();ite++)
			{
				string element = *ite;
				vector<string> listUsers;
				boost::split(listUsers,element,boost::is_any_of("&"));
				// users added to the map
				chatRoom[listUsers[1]] = listUsers[0];
				cout<<listUsers[0]<<"\t"<<listUsers[1]<<endl; 	
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
		cin.get(msgBuffer,500);		
		
		// send the message to the leader
		string msg = string(msgBuffer);
		int sendResult = sendMessage(clientFd,msg,leaderAddress);
		if(sendResult == -1)
	        {
        	        #ifdef DEBUG
                	cout<<"[DEBUG]message could not be sent to the leader"<<endl;
                	#endif
        	}
		// enque the message in the clientQueue
		
		// create an object of the message class
		Message m(msg);
		q.push(m);		
		
	}
}

void Client :: receiver()
{
	/* receiver thread should wait to receive the message from leader or from 
	other clients, verify and dequeue it from the blocking queue*/
	while(true)
	{
		socklen_t len = sizeof(clientAddress);
		//string msg = receivedMessage();		
	}
}
