#include "message.h"

using namespace std;

Message :: Message(int messageType,int seqNum, char* message)
{
	type = messageType;
	sequenceNumber = seqNum;
	buffer = message
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
