#include <iostream>
#include<string>
#include<cstring>
#include <boost/algorithm/string.hpp>
#include<map>
#include<vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "clientGlobal.h"

using namespace std;

class Client
{
	private:
	string userName;
	string leaderIp;
	int leaderPort;
	map<string,string> chatRoom;
	int clientFd;
	struct sockaddrIn leaderAddress, clientAddress;
        struct hostent *serverName;
        socklen_t leaderAddressLength;
        socklen_t clientAddressLength;

	public:
	Client(string name,string leaderIpPort);
	int establishConnection();
};

Client :: Client(string name,string leaderIpPort)
{
	userName = name;
       	vector<string> ipPortStr
        boost::split(split_string,leaderIpPort,boost::is_any_of(":"));
        leaderIp = ipPortStr[0];
        leaderPort =atoi(ipPortStr[1]);
}
int Client :: establishConnection()
{
	clientFd = socket(AF_INET,SOCK_DGRAM,0);
	if(clientFd < 0)
                {
                	cout<<"\nsocket could not be established\n"<<endl;
                        return 0;
                }
	
	bzero((char *) &serverAddress, sizeof(serverAddress));
        serverAddress.sin_family = AF_INET;
        inet_pton(AF_INET,leaderIp,&(serverAddress.sin_addr));
        serverAddress.sin_port = htons(leaderPort);
		
}
