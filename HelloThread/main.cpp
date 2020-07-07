#include <iostream>
#include <thread>
#include <mutex>	//锁
#include <atomic>
#include "CELLTimeStamp.hpp"
using namespace std;

//原子操作 原子 分子
mutex m;
const int tCount = 4;
atomic_int sum = 0;

void workFun(int index)
{
	for (int i = 0; i < 20000000; ++i)
	{
		//自解锁
		/*lock_guard<mutex> lg(m);*/

		/*m.lock();*/
		//临界区域-开始
		sum++;
		//临界区域-结束
		/*m.unlock();*/
	}
	

	/*cout << index << "Hello, other thread" << i << endl;*/
}//抢占式
int main()
{
	thread t[tCount];
	for (int i = 0; i < tCount; ++i)
	{
		t[i] = thread(workFun, i);
	}
	CELLTimestamp tTime;
	for (int i = 0; i < tCount; ++i)
	{
		t[i].join();
		//t[i].detach();
	}

	cout << tTime.getElapsedTimeInMilliSec() <<",sum = " << sum << endl;
	sum = 0;
	tTime.update();

	for (int i = 0; i < 80000000; ++i)
	{
		sum++;
	}
	cout << tTime.getElapsedTimeInMilliSec() << ",sum = " << sum << endl;

	cout << "Hello, main thread" << endl;

	return 0;
}