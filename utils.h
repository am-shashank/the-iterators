#include<string>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>

using namespace std;

string getIp();
// send message function: fd- socket_file_descriptor
int sendMessage(int fd,string msg,sockaddr_in addr);
// receive message which returns the message received
int receiveMessage(int fd,sockaddr_in *addr,socklen_t *addrLen, char* buffer);

/*
	Send Message with retries
	Parameters:
		sendFd - socketFd where the msg needs to be sent to
		recvFd - socketFd where the ACK needs to be received
		numReTry - number os retries remaining for sending the message
*/
int sendMessageWithRetry(int sendFd, string msg, sockaddr_in addr, int recvFd, int numRetry);

Id getId(struct sockaddr_in);
