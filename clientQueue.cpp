#include "globals.h"

void ClientQueue::push(const Message &m)
{
        unique_lock<mutex> mlock(qmtx);
        clientQueue.push(m);
        mlock.unlock();
        myCondition.notify_one();
}

Message ClientQueue::pop()
{
        unique_lock<mutex> mlock(qmtx);
        while (clientQueue.empty())
        {
                myCondition.wait(mlock);
        }
        Message item = clientQueue.front();
        clientQueue.pop();
        return item;
}

