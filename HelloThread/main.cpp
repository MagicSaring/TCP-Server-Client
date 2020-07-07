#include <iostream>
#include <thread>
#include <mutex>	//��
#include <atomic>
#include "CELLTimeStamp.hpp"
using namespace std;

//ԭ�Ӳ��� ԭ�� ����
mutex m;
const int tCount = 4;
atomic_int sum = 0;

void workFun(int index)
{
	for (int i = 0; i < 20000000; ++i)
	{
		//�Խ���
		/*lock_guard<mutex> lg(m);*/

		/*m.lock();*/
		//�ٽ�����-��ʼ
		sum++;
		//�ٽ�����-����
		/*m.unlock();*/
	}
	

	/*cout << index << "Hello, other thread" << i << endl;*/
}//��ռʽ
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