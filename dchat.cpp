#include<iostream>
#include "globals.h"
using namespace std;
int main(int argc,char *argv[])
{
	if(argc == 2)
	{
		//invoke the leader
		Leader leader(argv[1]);				
		Message m(JOIN, 1, "message 1");
		BlockingPQueue bpq;
		bpq.push(m);
		cout<<bpq.pop().getMessage();		
	}
	else if(argc == 3)
	{
		//invoke the client
		Client worker(argv[1],argv[2]);
	}
	else
	{
		cout<<"dchat <USER> [ADDR:PORT]"<<endl;
	}
	return 0;
}
