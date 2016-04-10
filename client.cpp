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
/*
function is used to set the attributes for the server
*/
void Client :: setServerAttributes(char* ip, int port)
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
	int portNo;
	if(clientFd < 0)
                {
                	cout<<"\nsocket could not be established\n"<<endl;
                        return 0;
                }

	 
	cout<<"socket created"<<endl;	
	
	// setting the attributes for the server
	setServerAttributes(leaderIp,leaderPort);
	cout<<"before setting client address"<<endl;
	// get local ip
        string clientIp = getIp();
        cout << "client ip:\t"<<clientIp<<endl;

	bzero((char *) &clientAddress, sizeof(clientAddress));
        clientAddress.sin_family = AF_INET;
        //clientAddress.sin_addr.s_addr = inet_addr(INADDR_ANY);
	inet_pton(AF_INET,clientIp.c_str(),&(clientAddress.sin_addr));
	cout<<"before bind"<<endl;
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
	cout<<"after bind"<<endl;

        clientAddress.sin_port = htons(portNo);

	/* binding the client to the socket
	if(bind(clientFd,(struct sockaddr *)&clientAddress, sizeof(clientAddress))<0)
        {
            cout<<"\nerror occurred while binding the client to the socket\n"<<endl;
            exit(1);
        }*/

	cout<<"client listening"<<endl;	
	
	cout<<"my local port is\t"<<portNo<<endl;
	joinNetwork(portNo,clientIp);	
		
}

void Client :: joinNetwork(int portNo,string clientIp)
{
	// calculate local ip
	
	socklen_t len = sizeof(clientAddress);

	std::stringstream joinMsg;

	joinMsg<< JOIN<<"%" << userName <<"%" <<clientIp<<":"<<portNo;
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
		cout<<"Message received\t"<<receivedMessage<<endl;
	}
	else
	{
		#ifdef DEBUG
		cout<<"[DEBUG]message could not be recived\n"<<endl;
		#endif
	}
				
} 
