#include "globals.h"

using namespace std;

Message :: Message(int messageType,int seqNum, char* message)
{
	type = messageType;
	sequenceNumber = seqNum;
	buffer = message;
}
int Message :: getType()
{
	return type;
}

int Message :: getSequenceNumber()
{
	return sequenceNumber;
}
char* Message :: getMessage()
{
	return buffer;
}

/*
	Overloading less than operator
	to assist in sorting of messages
	in a priority Queue
*/
bool Message::operator<(const Message &m) const 
{
	return type - m.type;
}

