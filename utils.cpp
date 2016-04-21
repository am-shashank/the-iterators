#include <stdio.h>      
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>
#include <string>
#include "globals.h"
#include<iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#define MIN_PORTNO 2000
#define MAX_PORTNO 50000


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

// method to randomly generate a port number

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

	

