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
#define ACK 6
#define ELECTION 7
#define ADD_USER 8
#define CHAT 100
#define DEQUEUE 99
// threshold for heart-beat in milliseconds
#define HEARTBEAT_THRESHOLD 4000 // frequencey at which heart beats are sent in milli seconds 
#define NUM_RETRY 3 // Number of retries for message
#define TIMEOUT_RETRY 5 // Timeout for retrying sending of messages ***in seconds***
#define IS_LEADER 0 // 0 indicates - client, 1 - indicates Leader
#define NODE_DEAD -100

using namespace std;
class Message
{
	private:
	int type;
	int sequenceNumber;
	string buffer;
	int msgId;

	public:

	Message(string message);	
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
		
	ChatRoomUser();
	ChatRoomUser(string name, string ip, int port, int ackPort, int heartbeatPort);
};

class Id {
	public:
		string ip;
		int port;
		Id(string ip1, int port1); 
		Id(string IpPort);
		bool operator <(const Id &id2) const;
		friend ostream& operator<<(ostream& o, const Id &obj) {  
        		o<<obj.ip<<":"<<obj.port;
			return o;
		}	

};
Id getId(struct sockaddr_in clientAdd);
Id receiveMessageWithAck(int sockFd, int ackFd, map<Id, ChatRoomUser> chatRoom, char* buffer);


class Leader : public ChatRoomUser
{
	int seqNum;   // global sequence number for ordering of messages
	BlockingPQueue q;  // Blocking Priority Queue to maintain incoming messages to be broadcasted
	
	int sockFd, heartbeatSockFd, ackSockFd;
	struct sockaddr_in sock, heartbeatSock, ackSock;
	
	// map of users ip and names in chat room
	map<Id, ChatRoomUser> chatRoom;
	map<Id, int> lastSeenMsgIdMap;

	public:
		Leader(string leaderName); 
		void startServer();
		void printStartMessage();
		// Thread task to listen for messages in chatroom from participants
		void producerTask(int fd);
		// Thread task to multi-cast messages to participants in chatroom
		void consumerTask();
		// parse the incoming message and take appropriate actions
		void parseMessage(char *message, Id clientId);
		// Sending the list of current users in chatroom to the client specified
		void sendListUsers(string clientIp, int clientPort);
		// to send recieve heartbeats, detect failures
		void heartbeatSender();
		void heartbeatReceiver();
		void detectClientFaliure();
		// delete the user if he exits/ crashes
		void deleteUser(Id clientId);
		// receive a message and send an ack with the message id
};

class Client
{

        private:
        string userName;
        char* leaderIp;
        int leaderPort;
	//int leaderAckPort;
	//int leaderHeartBeatPort;
	int clientPort;
	int heartBeatPort;
	int ackPort;
	string clientIp;
	bool isLeader;
	// variable to check last seen msg and sequence number
	string lastSeenMsg;
	int lastSeenSequenceNum;
	// client socket descriptor
        int clientFd;
	// client socket descriptor for heart beat and acknowledgements
	int heartBeatFd;
	int ackFd;
	// declare a message id which would be unique for every message sent by the client
	int msgId;
        struct sockaddr_in leaderAddress,leaderAckAddress,leaderHeartBeatAddress,clientAddress, heartBeatAddress, ackAddress;
        socklen_t leaderAddressLength;
        socklen_t clientAddressLength;

	// hash map to store list of active users       
        map<Id,ChatRoomUser> chatRoom;

	// objects for blocking queue for client
        ClientQueue q;

        public:
        Client(string name,string leaderIpPort);
        int establishConnection();
	void setupClientPorts();
	void setupLeaderPorts(int lAckPort,int heartbeatPort);
	void setLeaderAttributes(char* ip, int port);
        int joinNetwork(int portNo,string localIp);
	void sender();
	void receiver();
	//void processReceivedMessage();
	void sendHeartbeat();
	void detectLeaderFailure();
	void exitChatroom();	
	void sendAck(string msg);
		
};

