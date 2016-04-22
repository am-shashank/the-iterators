#include "globals.h"
Id::Id(string IpPort) {
	vector<string> ipPortSplit;
       	boost::split(ipPortSplit,IpPort,boost::is_any_of(":"));
       	ip = ipPortSplit[0];
        port = atoi(ipPortSplit[1].c_str());
}

Id::Id(string ip1, int port1) {
	ip = ip1;
        port = port1;
}

Id::operator string() const { 
	return ip + ":" + to_string(port); 
}
               
bool Id:: operator <(const Id &id2) const {
	if(ip == id2.ip) 
		return port < id2.port;
	return ip < id2.ip;
}

Id getId(struct sockaddr_in addr) {
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr.sin_addr), ip, INET_ADDRSTRLEN);
        return Id(string(ip), ntohs(addr.sin_port));
}

