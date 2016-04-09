int main(int argc,char *argv[])
{
	if(argc == 2)
	{
		//invoke the leader
		Leader leader(argv[1]);				

	}
	else if(argc == 3)
	{
		//invoke the client
		Client worker(argv[1],argv[2]);
	}
	else
	{
		cout<<"Wrong invocation...Please try again\n"<<endl;
	}
}