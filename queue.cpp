#include "globals.h"

void BlockingPQueue::push(const Message &m)
{
	unique_lock<mutex> mlock(mtx);
    	pQueue.push(m);
    	mlock.unlock();
    	conditionVar.notify_one();
}

Message BlockingPQueue::pop()
{
	unique_lock<mutex> mlock(mtx);
    	while (pQueue.empty())
    	{
     	 	conditionVar.wait(mlock);
    	}
    	Message item = pQueue.top();
    	pQueue.pop();
    	return item;
}
int BlockingPQueue::size() 
{
	return pQueue.size();
}
