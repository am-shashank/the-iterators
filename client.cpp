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
#include "utils.h"
#include "globals.h"

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
	clientFd = socket(AF_INET,SOCK_DGRAM,0);
	if(clientFd < 0)
                {
                	cout<<"\nsocket could not be established\n"<<endl;
                        return 0;
                }
	
	bzero((char *) &leaderAddress, sizeof(leaderAddress));
        leaderAddress.sin_family = AF_INET;
        inet_pton(AF_INET,leaderIp,&(leaderAddress.sin_addr));
        leaderAddress.sin_port = htons(leaderPort);
	
	joinNetwork();	
		
}

void Client :: joinNetwork()
{
	// calculate local ip
				
} 
