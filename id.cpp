#include "globals.h"
Id::Id(stringIpPort) {
	vector<string> ipPortSplit;
       	boost::split(ipPortSplit,messageSplit[2],boost::is_any_of(":"));
       	ip = ipPortSplit[0];
        port = atoi(ipPortSplit[1].c_str());
}

Id::Id(string ip1, int port1) {
	ip = ip1;
        port = port1;
}

operator Id::string() const { 
	return ip + ":" + to_string(port); 
}
               
bool Id:: operator <(const Id &id2) const {
	if(ip == id2.ip) 
		return port < id2.port;
	return ip < id2.ip;
}
