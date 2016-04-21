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
#define NUM_RETRY 3 // Number of retries for message
#define TIMEOUT_RETRY 5 // Timeout for retrying sending of messages ***in seconds***
#define IS_LEADER 0 // 0 indicates - client, 1 - indicates Leader
#define HEARTBEAT_THRESHOLD 10 // threshold for heart-beat

using namespace std;

class Message
{
	private:
	int type;
	int sequenceNumber;
	string buffer;
	int msgId;

	public:
	
	Message(int messageId,string message);
	Message(int messageType,int seqNum,string message);
	int getType();
	int getSequenceNumber();
	int getMessageId();
	string getMessage();
	void setMessage(string message);
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
class ClientQueue
{
	private:

	queue<Message> clientQueue;
	mutex qmtx;
	condition_variable myCondition;
	
	public:

	void push(const Message &m);
	Message pop();
	
};


class Id {
	public:
		string ip;
		int port;
		Id(string ip1, int port1); 
		Id(string IpPort);
		operator string() const ;
		bool operator <(const Id &id2) const;
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

	int heartbeatPortNo;
	// heartbeat socket info
	int heartbeatFd;
	struct sockaddr_in heartbeatAdd;
		
	int seqNum;   // global sequence number for ordering of messages
	BlockingPQueue q;  // Blocking Priority Queue to maintain incoming messages to be broadcasted

	// map of users ip and names in chat room
	map<Id, ChatRoomUser> chatRoom;

	// map of users ip:port and ip:ackPort 
	map<Id, Id> ackMap;

	// map of users ip:port and ip:heartbeat_port
	map<Id, Id> heartbeatMap; 

	// map of client heartbeats in chat room
	map<Id, chrono::time_point<chrono::system_clock>> clientBeat;

	public:
		Leader(string leaderName); 
		void startServer();
		void printStartMessage();
		
		// Thread task to listen for messages in chatroom from participants
		void producerTask(int fd);
		
		// Thread task to multi-cast messages to participants in chatroom
		void consumerTask();
		// parse the incoming message and take appropriate actions
		void parseMessage(char *message, string clientIp, int clientPort);
		// Sending the list of current users in chatroom to the client specified
		void sendListUsers(string clientIp, int clientPort);
		// to send recieve heartbeats, detect failures
		void heartbeatSender();
		// void heartbeatReciever();
		void detectClientFaliure();
		// delete the user if he exits/ crashes
		void deleteUser(Id clientId);
};

/*
	Maintain all the information about the chatroom user
*/
class ChatRoomUser {
	public:	
		int port; // port for broadcast messages
		int ackPort; // port for Acknowledgements
		int heartbeatPort; // port for heart beats
		string ip;
		string name; // user name of the chatroom user
		chrono::time_point<chrono::system_clock> lastHbt; // time when the last heartbeat was received
		 
	ChatRoomUser(int port, int ackPort, int heartbeatPort, string ip, string name) {
		this.port = port;
		this.ackPort = ackPort;
		this.heartbeatPort = heartbeatPort;
		this.ip = ip;
		this.name = name;
		this.lastHbt = chrono::system_clock::now();
	}
};


class Client
{
	private:
		string userName;
		char* leaderIp;
		int leaderPort;
		int clientPort;
		string clientIp;
		bool isLeader;
		// client socket descriptor
		int clientFd;
		// declare a message id which would be unique for every message sent by the client
		int msgId;
		struct sockaddr_in leaderAddress, clientAddress;
		socklen_t leaderAddressLength;
		socklen_t clientAddressLength;

		// hash map to store list of active users       
		map<string,string> chatRoom;

		// object for blocking queue for client
		ClientQueue q;

	public:
		Client(string name,string leaderIpPort);
		int establishConnection();
		void setLeaderAttributes(char* ip, int port);
		int joinNetwork(int portNo,string localIp);
		void sender();
		void receiver();
		void exitChatroom();	
};


