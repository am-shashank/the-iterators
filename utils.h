#include<string>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>

using namespace std;

string getIp();
// send message function: fd- socket_file_descriptor
int sendMessage(int fd,string msg,sockaddr_in addr);
// receive message which returns the message received
string receiveMessage(int fd,sockaddr_in *addr,socklen_t *addrLen);
