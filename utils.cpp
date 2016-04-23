#include <stdio.h>      
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <string> 
#include <arpa/inet.h>
#include <string>
#include<iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#define MIN_PORTNO 2000
#define MAX_PORTNO 50000
#include "globals.h"

using namespace std;

string getIp(){
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
	    if(strcmp("em1", ifa->ifa_name) == 0) {
            	string str(addressBuffer);
		#ifdef DEBUG
		cout<<"[DEBUG]"<<str<<endl;
		#endif
		return str;
	    }
        } else if (ifa->ifa_addr->sa_family == AF_INET6) { // check it is IP6
            // is a valid IP6 Address
            tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
            if(strcmp("em1", ifa->ifa_name) == 0) {
		#ifdef DEBUG
            	printf("[DEBUG] %s IP Address %s\n", ifa->ifa_name, addressBuffer); 
		#endif
		string str(addressBuffer);
		return str;
	    }
	} 
    }
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
    
}

// function used to send message
int sendMessage(int fd,string msg,sockaddr_in addr)
{
	char writeBuffer[500];
	bzero(writeBuffer,501);
	socklen_t len = sizeof(addr);
	strncpy(writeBuffer,msg.c_str(),sizeof(writeBuffer));
        int result = sendto(fd,writeBuffer,strlen(writeBuffer),0,(struct sockaddr *)&addr,len);
	return result;

}
// receive message which returns the message received
int receiveMessage(int fd,sockaddr_in *addr,socklen_t *addrLen,char* buffer)
{

	//char readBuffer[500];
	//string msg = "";
	//bzero(readBuffer,501);
	int num_char = recvfrom(fd,buffer,500,0,(struct sockaddr *) addr,addrLen);
	#ifdef DEBUG
	//cout<<"[DEBUG]Inside utils.cpp message received\t"<<buffer<<endl;
	#endif
	if(num_char<0)
        {
        	cout<<"error reading from socket\n"<<endl;
        }
	return num_char;
	
}

/*
	Receive a message and acknowledge it
        Parameters:
		sockFd : socket fd where the user is listening for messages
		ackFd : socket where chatroomuser is sending ACK
		chatRoom : map of ip:port and other chatRooms	
                buffer: buffer where the resulting message is put
	Return:
		Id of the chatroomuser who sent the message 
*/
Id receiveMessageWithAck(int sockFd, int ackFd, map<Id, ChatRoomUser> chatRoom, char* buffer)
{
        struct sockaddr_in clientAdd;
        bzero((char *) &clientAdd, sizeof(clientAdd));
        socklen_t clientLen = sizeof(clientAdd);
        int num_char = recvfrom(sockFd, buffer, 500, 0, (struct sockaddr *) &clientAdd, &clientLen);
	
        // spliting the encoded message
        vector<string> messageSplit;
        boost::split(messageSplit, buffer, boost::is_any_of("%"));
        int msgCode = atoi(messageSplit[0].c_str());
	
	Id clientId;
	int ackPort;
	if(msgCode == JOIN) {
		clientId = Id(messageSplit[3]);
		ackPort = atoi(messageSplit[4].c_str());
	} else {
		clientId = getId(clientAdd);	
		ackPort = chatRoom[clientId].ackPort;
	}
	int msgId = atoi(messageSplit[1].c_str());

        // send ACK to clientAckPort with Message Id
        /* set socket attributes for participant's ACK port */
        struct sockaddr_in clientAckAdd;
        bzero((char *) &clientAckAdd, sizeof(clientAckAdd));
        clientAckAdd.sin_family = AF_INET;
        inet_pton(AF_INET,clientId.ip.c_str(),&(clientAckAdd.sin_addr));
        clientAckAdd.sin_port = htons(ackPort);
        sendMessage(ackFd, to_string(ACK) + "%" + to_string(msgId), clientAckAdd);
        #ifdef DEBUG
        cout<<"[DEBUG]ACK sent for "<<buffer<<endl;
        #endif
        return clientId;
}


/*
	Bind the specified socket to a random
	port number that is available	
*/
int generatePortNumber(int fd,sockaddr_in &addr)
{
    struct timeval t1;
    int portNum;
    gettimeofday(&t1, NULL);
    srand(t1.tv_usec * t1.tv_sec);

    while(true) {

    	int range = MAX_PORTNO - MIN_PORTNO + 1;
        portNum = rand() % range + MIN_PORTNO;
        #ifdef DEBUG
       	//cout<<"[DEBUG]client port generated\t"<<clientPort<<endl;
        #endif
        addr.sin_port = htons(portNum);
        #ifdef DEBUG
        //cout <<"[DEBUG] sin_port generated\t"<<htons(clientPort)<<endl;
        #endif

        if(bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        	cerr << "Error: Cannot bind socket on " <<portNum<<endl;
        }else
        	break;
    }
	return portNum;
}

/*
        Send Message with retries ensuring reliable delivery
        Parameters:
                sendFd - socketFd of the sender
		msg - Message to be sent
		addr - socket of the recepient
                ackFd - socketFd where the ACK needs to be received
                numReTry - number os retries remaining for sending the message
	Return:
		number of bytes sent / NODE_DEAD if the node is dead
*/
int sendMessageWithRetry(int sendFd, string msg, sockaddr_in addr, int ackFd, int numRetry) 
{
	if(numRetry == 0) 
		return NODE_DEAD;
			
	char writeBuffer[500];
	bzero(writeBuffer,501);
	socklen_t len = sizeof(addr);
	strncpy(writeBuffer,msg.c_str(),sizeof(writeBuffer));
	#ifdef DEBUG
	cout<<"[DEBUG] Sending "<<msg<<endl;
	#endif

	int result = sendto(sendFd,writeBuffer,strlen(writeBuffer),0,(struct sockaddr *)&addr,len);
	
	// wait for ACK with timeout
	struct sockaddr_in clientAdd;
	socklen_t clientLen = sizeof(clientAdd);
	char readBuffer[500];
	bzero(readBuffer, 501);
	struct timeval timeout={ TIMEOUT_RETRY, 0}; //set timeout for 2 seconds
	setsockopt(ackFd,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));
	int recvLen = recvfrom(ackFd, readBuffer, 500, 0, (struct sockaddr *) &clientAdd, &clientLen);
	// Message Receive Timeout or other error. Resend Message
	if (recvLen <= 0) {
		#ifdef DEBUG
		cout<<"[DEBUG] Sending "<<msg<<" failed";
		#endif 
		sendMessageWithRetry(sendFd, msg, clientAdd, ackFd, numRetry - 1);
	}
	#ifdef DEBUG
	cout<<"[DEBUG] ACK received for "<<msg<<endl;
	#endif
	return result;
}
