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
        socklen_t leaderAddressLength;
        socklen_t clientAddressLength;
        
        public:
        Client(string name,string leaderIpPort);
        int establishConnection();
        void joinNetwork();
};
class Message
{
	private:
	int type;
	int sequenceNumber;
	char* buffer;

	public:

	Message(int messageType,int seqNum,char* message);
	int getType();
	int getSequenceNumber();
	char* getMessage();
	int sendMessage();
	int receiveMessage();
	
};
