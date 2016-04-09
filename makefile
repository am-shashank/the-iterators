all: 	dchat.o leader.o client.o
	g++ -o dchat dchat.o leader.o utils.o client.o
dchat.o: dchat.cpp 
	g++ -c dchat.cpp
leader.o:leader.cpp utils.o globals.h
	g++ -c leader.cpp 
client.o:client.cpp utils.o globals.h
	g++ -c client.cpp
utils.o:utils.cpp utils.h
	g++ -c utils.cpp
clean:
	rm -f dchat dchat.o leader.o client.o utils.o
	
