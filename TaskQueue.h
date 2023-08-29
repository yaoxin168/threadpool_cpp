#pragma once
#include <queue>
#include <pthread.h>//mutex
using namespace std;

using callback = void (*)(void* arg);
//����
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
	callback function;//����ָ�룬����ֵΪvoid
	T* arg;//�����Ĳ���
};

//�������
template<typename T>
class TaskQueue
{
public:
	TaskQueue();
	~TaskQueue();

	//�������
	void addTask(Task<T> task);
	void addTask(callback f, void*arg);

	//ȡ����
	Task<T> takeTask();

	//��ǰ��������е��������
	inline size_t taskNumber()
	{
		return m_taskQ.size();
	}

private:
	pthread_mutex_t m_mutex;//������
	queue<Task<T>> m_taskQ;//�����Լ�ά�����ζ��У�����ͷβ�ˡ�����ֱ��ʹ������


};

