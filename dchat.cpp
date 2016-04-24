#include<iostream>
#include "globals.h"
using namespace std;
int main(int argc,char *argv[])
{
	if(argc == 2)
	{
		//invoke the leader
		#ifdef DEBUG
		cout<<"Invoke the leader"<<endl;
		#endif
		Leader leader(argv[1]);				
	}
	else if(argc == 3)
	{
		//invoke the client
		#ifdef DEBUG
		cout<<"Invoking the client"<<endl;
		#endif
		Client worker(argv[1],argv[2]);
	}
	else
	{
		cout<<"dchat <USER> [ADDR:PORT]"<<endl;
	}
	return 0;
}
