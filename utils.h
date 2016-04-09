#include<string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
 
string getIp();


class Queue
{
  
	private:
  		std::queue<T> queue_;
  		std::mutex mutex_;
  		std::condition_variable cond_;

	public:
				
 
};
