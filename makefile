CPPFLAGS = -std=c++11 -lpthread
all: 	dchat.o leader.o client.o
	g++ $(CPPFLAGS) -o dchat dchat.o leader.o utils.o client.o queue.o message.o clientQueue.o id.o chatRoomUser.o 
 
dchat.o: dchat.cpp
	g++ $(CPPFLAGS) -c dchat.cpp
leader.o:leader.cpp utils.o globals.h queue.o chatRoomUser.o clientQueue.o 
	g++ $(CPPFLAGS) -c leader.cpp 
client.o:client.cpp utils.o globals.h clientQueue.o 
	g++ $(CPPFLAGS) -c client.cpp
utils.o:utils.cpp utils.h globals.h id.o
	g++ $(CPPFLAGS) -c utils.cpp
queue.o:queue.cpp globals.h message.o
	g++ $(CPPFLAGS) -c queue.cpp
clientQueue.o:clientQueue.cpp globals.h message.o 
		g++ $(CPPFLAGS) -c clientQueue.cpp	
message.o:message.cpp globals.h
	g++ $(CPPFLAGS) -c message.cpp
chatRoomUser.o:chatRoomUser.cpp globals.h
	g++ $(CPPFLAGS) -c chatRoomUser.cpp
id.o:id.cpp utils.h globals.h 
	g++ $(CPPFLAGS) -c id.cpp
clean:
	rm -f dchat dchat.o leader.o client.o utils.o queue.o message.o clientQueue.o chatRoomUser.o id.o semaphore.o
	
