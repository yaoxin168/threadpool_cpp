#pragma once
#include <queue>
#include <pthread.h>//mutex
using namespace std;

using callback = void (*)(void* arg);
//任务
template<typename T>
struct Task
{
	Task<T>()
	{
		function = nullptr;
		arg = nullptr;
	}
	Task<T>(callback f, void* arg)
	{
		this->arg = (T*)arg;
		function = f;
	}
	callback function;//函数指针，返回值为void
	T* arg;//函数的参数
};

//任务队列
template<typename T>
class TaskQueue
{
public:
	TaskQueue();
	~TaskQueue();

	//添加任务
	void addTask(Task<T> task);
	void addTask(callback f, void*arg);

	//取任务
	Task<T> takeTask();

	//当前任务队列中的任务个数
	inline size_t taskNumber()
	{
		return m_taskQ.size();
	}

private:
	pthread_mutex_t m_mutex;//互斥锁
	queue<Task<T>> m_taskQ;//不再自己维护环形队列，保存头尾了。而是直接使用容器


};

