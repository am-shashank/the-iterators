#include "globals.h"

void BlockingPQueue::push(Message m)
{
	unique_lock<mutex> mlock(mtx);
    	pQueue.push(m);
    	mlock.unlock();
    	conditionVar.notify_one();
}

Message pop()
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
