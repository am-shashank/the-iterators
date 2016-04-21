#include<string>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>

using namespace std;

string getIp();
//method to randomly generate port number
int generatePortNumber(int fd,sockaddr_in &addr);

// send message function: fd- socket_file_descriptor
int sendMessage(int fd,string msg,sockaddr_in addr);

// receive message which returns the message received
int receiveMessage(int fd,sockaddr_in *addr,socklen_t *addrLen, char* buffer);

// function to send heartbeat periodically
void sendHeartbeat(int fd,sockaddr_in addr);
