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

// For implementing Blocking Priority Queue
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

#define DEBUG 1	

// chat priority codes
#define JOIN 1
#define DELETE 2
#define HEARTBEAT 3
#define RESOLVE_LEADER 4
#define LIST_OF_USERS 5
#define CHAT 100

using namespace std;

class Message
{
	private:
	int type;
	int sequenceNumber;
	string buffer;

	public:

	Message(int messageType,int seqNum,string message);
	int getType();
	int getSequenceNumber();
	string getMessage();
        bool operator<(const Message &m1) const;
	
	~Message(); 
};


class BlockingPQueue
{
	priority_queue<Message> pQueue; 
  	mutex mtx;
  	condition_variable conditionVar;
	public:
		void push(const Message &m);
		Message pop();

};

class Leader 
{

	string name;
	string ip; 
	int portNo;
        	
	// socket specific info
	int socketFd;
	struct sockaddr_in svrAdd;
	socklen_t len; //store size of the address
		
	int seqNum;   // global sequence number for ordering of messages
	BlockingPQueue q;

	// map of users ip and names in chat room
	map<string, string> chatRoom;
	public:
		Leader(string leaderName); 
		void startServer();
		void printStartMessage();
		
		// Thread task to listen for messages in chatroom from participants
		void producerTask();
		
		// Thread task to multi-cast messages to participants in chatroom
		void consumerTask();
		void parseMessage(char *message);
		void sendListUsers(string clientIp, int clientPort);
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
	void setServerAttributes(char* ip, int port);
        void joinNetwork(int portNo,string localIp);
};


