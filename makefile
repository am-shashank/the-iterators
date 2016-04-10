CPPFLAGS = -std=c++11 
all: 	dchat.o leader.o client.o
	g++ $(CPPFLAGS) -o dchat dchat.o leader.o utils.o client.o
dchat.o: dchat.cpp 
	g++ $(CPPFLAGS) -c dchat.cpp
leader.o:leader.cpp utils.o globals.h queue.o
	g++ $(CPPFLAGS) -c leader.cpp 
client.o:client.cpp utils.o globals.h
	g++ $(CPPFLAGS) -c client.cpp
utils.o:utils.cpp utils.h
	g++ $(CPPFLAGS) -c utils.cpp
queue.o:queue.cpp globals.h message.o
	g++ $(CPPFLAGS) -c queue.cpp
message.o:message.cpp globals.h
	g++ $(CPPFLAGS) -c message.cpp
clean:
	rm -f dchat dchat.o leader.o client.o utils.o queue.o
	
