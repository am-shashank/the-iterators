#include "globals.h"

using namespace std;

Message :: Message(string message)
{
	buffer = message;
}
Message :: Message(int messageType,int seqNum,string message)
{
	type = messageType;
	sequenceNumber = seqNum;
	buffer = message;
}
Message :: Message(int messageId,string message)
{
	msgId = messageId;
	buffer = message;
}
int Message :: getType()
{
	return type;
}
int Message :: getMessageId()
{
	return msgId;
}
int Message :: getSequenceNumber()
{
	return sequenceNumber;
}
string Message :: getMessage()
{
	return buffer;
}

void Message :: setMessage(string message)
{
	buffer = message;
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



Message::~Message() 
{
}
