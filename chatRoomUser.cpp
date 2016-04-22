#include "globals.h"

ChatRoomUser:: ChatRoomUser(string name, string ip, int port, int ackPort, int heartbeatPort) {
    this->name = name;
    this->ip = ip;
    this->port = port;
    this->ackPort = ackPort;
    this->heartbeatPort = heartbeatPort;
    this->lastHbt = chrono::system_clock::now();
}
	
ChatRoomUser:: ChatRoomUser()
{}
