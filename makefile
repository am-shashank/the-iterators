all: 	dchat.o
	g++ -o dchat dchat.o leader.o
dchat.o: dchat.cpp leader.o
	g++ -c dchat.cpp
leader.o:leader.cpp utils.o
	g++ -c leader.cpp
client.o:client.cpp utils.o
	g++ -c client.cpp
utils.o:utils.cpp
	g++ -c utils.cpp
clean:
	rm -f dchat dchat.o leader.o client.o utils.o
	
