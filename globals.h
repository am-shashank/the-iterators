#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <boost/algorithm/string.hpp>
#include<map>

// chat codes
#define JOIN 1
#define DELETE 2
#define HEARTBEAT 3
using namespace std;


class Leader {

	string name;
	string ip; 
	int portNo;
	
	// socket specific info
	int socketFd;
	struct sockaddr_in svrAdd;
	socklen_t len; //store size of the address

	// map of users ip and names in chat room
	map<string, string> chatRoom;
	public:
		Leader(string leaderName); 
		int startServer();
		void printStartMessage();
};
class Client
{
        private:
        string userName;
        char* leaderIp;
        int leaderPort;
        map<string,string> chatRoom;
        int clientFd;
        struct sockaddr_in leaderAddress, clientAddress;
        struct hostent *serverName;
        struct ifreq ifr;
        socklen_t leaderAddressLength;
        socklen_t clientAddressLength;
        char writeBuffer[256];
        char readBuffer[256];
        public:
        Client(string name,string leaderIpPort);
        int establishConnection();
        void joinNetwork();
};

