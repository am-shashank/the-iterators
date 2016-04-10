#include <iostream>
#include<string>
#include<cstring>
#include <boost/algorithm/string.hpp>
#include<map>
#include<vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include<sstream>
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
int Client :: establishConnection()
{
	//create socket

	clientFd = socket(AF_INET,SOCK_DGRAM,0);
	int flag=1;
	int portNo;
	if(clientFd < 0)
                {
                	cout<<"\nsocket could not be established\n"<<endl;
                        return 0;
                }

	// making the port number reusable: multiple sockets using same port
	if(setsockopt(clientFd,SOL_SOCKET,SO_REUSEADDR,&flag,sizeof(flag))<0)
	{
		cout<<"setsockopt() command failed\n"<<endl;
			
	} 
	
	bzero((char *) &leaderAddress, sizeof(leaderAddress));
        leaderAddress.sin_family = AF_INET;
        inet_pton(AF_INET,leaderIp,&(leaderAddress.sin_addr));
        leaderAddress.sin_port = htons(leaderPort);

	bzero((char *) &clientAddress, sizeof(clientAddress));
        clientAddress.sin_family = AF_INET;
        clientAddress.sin_addr.s_addr = inet_addr(INADDR_ANY);
	// randomly generated port of client
	while(true) {

                int range = MAX_PORTNO - MIN_PORTNO + 1;
                portNo = rand() % range + MIN_PORTNO;
                clientAddress.sin_port = htons(portNo);

                if(bind(clientFd, (struct sockaddr *)&clientAddress, sizeof(clientAddress)) < 0) {
                        cerr << "Error: Cannot bind socket on " <<portNo<<endl;
                }else
                        break;

        }

        clientAddress.sin_port = htons(portNo);

	// binding the client to the socket
	if(bind(clientFd,(struct sockaddr *)&clientAddress, sizeof(clientAddress))<0)
        {
            cout<<"\nerror occurred while binding the client to the socket\n"<<endl;
            exit(1);
        }

	cout<<"client listening"<<endl;	
	// get local ip
	string clientIp = getIp();
	cout << "client ip:\t"<<clientIp<<endl;

	// get local port number
 	socklen_t clientAddrLen = sizeof(clientAddress);
	/*
	if(getsockname(clientFd,(struct sockaddr *)&clientAddress,&clientAddrLen)==0 && clientAddress.sin_family == AF_INET && clientAddrLen == sizeof(clientAddress))
	{
		int localPort = ntohs(clientAddress.sin_port);
		cout<<"my local port is\t"<<localPort<<endl;		
	}
	else
	{
		cout<<"error occurred while calculating the port number of the client's machine\n"<<endl;
	}
	*/
	cout<<"my local port is\t"<<portNo<<endl;
	joinNetwork(portNo,clientIp);	
		
}

void Client :: joinNetwork(int portNo,string clientIp)
{
	// calculate local ip
	char writeBuffer[256];
	char readBuffer[256];
	socklen_t len = sizeof(clientAddress);

	bzero(writeBuffer,0);
	std::stringstream joinMsg;

	joinMsg<< JOIN << userName <<"%" <<clientIp<<":"<<portNo;
	string msg = joinMsg.str();
	strncpy(writeBuffer,msg.c_str(),sizeof(writeBuffer));
	sendto(clientFd,writeBuffer,strlen(writeBuffer),0,(struct sockaddr *)&leaderAddress,sizeof(leaderAddress));
        cout<<"message sent"<<endl;
        bzero(readBuffer,256);
        int numChar = recvfrom(clientFd,readBuffer,sizeof(readBuffer),0,(struct sockaddr *)&clientAddress,&len);
        if(numChar<0)
         {
                 cout<<"\nerror reading from socket\n"<<endl;

                 exit(1);
         }

	cout<<"message read\t"<<readBuffer<<endl;

				
} 
