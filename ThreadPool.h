#pragma once
#include "TaskQueue.h"
#include "TaskQueue.cpp"
template<typename T>
class ThreadPool
{
public:
	//创建线程池并初始化
	ThreadPool(int min, int max);
	//销毁线程池
	~ThreadPool();

	//添加任务到线程池
	void addTask(Task<T> task);

	//获得当前正在工作的线程数
	int getBusyNum();

	//活着的线程数
	int getAliveNum();

private:
	//这是写成静态的原因：在构造函数中有个地方需要传递worker函数的地址，
	//如果非静态，那么必须在该类实例化后函数才有地址。而对于非成员函数、静态成员函数，只要定义出来，就有地址。
	static void* worker(void* arg);//工作线程的函数
	static void* manager(void* arg);//管理者线程的函数
	void threadExit();//销毁线程，并重置threadIDs对应位置的值为0，便于存储后续新增工作线程id



private:
	TaskQueue<T>* taskQ;//任务队列，由队头队尾下标维护成一个环形队列

	pthread_t managerID;//管理者线程ID，管理者线程只有一个
	pthread_t* threadIDs;//工作的线程ID，有多个工作线程
	int minNum;//最小线程数
	int maxNum;//最大线程数
	int busyNum;//当前在工作的线程个数
	int liveNum;//当前存活的线程个数
	int exitNum;//如果大部分线程都闲着，需要杀死多少个线程

	pthread_mutex_t mutexPool;//互斥锁，锁整个线程池
	//不阻塞生产者，现在的任务队列容器是无限的了
	pthread_cond_t notEmpty;//任务队列不为空时唤醒工作线程(消费者)

	static const int NUMBER = 2;//写成静态是为了静态成员函数能直接访问它

	bool shutdown;//要不要销毁线程池，销毁为1，不销毁为0

};

