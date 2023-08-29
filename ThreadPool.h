#pragma once
#include "TaskQueue.h"
#include "TaskQueue.cpp"
template<typename T>
class ThreadPool
{
public:
	//�����̳߳ز���ʼ��
	ThreadPool(int min, int max);
	//�����̳߳�
	~ThreadPool();

	//��������̳߳�
	void addTask(Task<T> task);

	//��õ�ǰ���ڹ������߳���
	int getBusyNum();

	//���ŵ��߳���
	int getAliveNum();

private:
	//����д�ɾ�̬��ԭ���ڹ��캯�����и��ط���Ҫ����worker�����ĵ�ַ��
	//����Ǿ�̬����ô�����ڸ���ʵ�����������е�ַ�������ڷǳ�Ա��������̬��Ա������ֻҪ������������е�ַ��
	static void* worker(void* arg);//�����̵߳ĺ���
	static void* manager(void* arg);//�������̵߳ĺ���
	void threadExit();//�����̣߳�������threadIDs��Ӧλ�õ�ֵΪ0�����ڴ洢�������������߳�id



private:
	TaskQueue<T>* taskQ;//������У��ɶ�ͷ��β�±�ά����һ�����ζ���

	pthread_t managerID;//�������߳�ID���������߳�ֻ��һ��
	pthread_t* threadIDs;//�������߳�ID���ж�������߳�
	int minNum;//��С�߳���
	int maxNum;//����߳���
	int busyNum;//��ǰ�ڹ������̸߳���
	int liveNum;//��ǰ�����̸߳���
	int exitNum;//����󲿷��̶߳����ţ���Ҫɱ�����ٸ��߳�

	pthread_mutex_t mutexPool;//���������������̳߳�
	//�����������ߣ����ڵ�����������������޵���
	pthread_cond_t notEmpty;//������в�Ϊ��ʱ���ѹ����߳�(������)

	static const int NUMBER = 2;//д�ɾ�̬��Ϊ�˾�̬��Ա������ֱ�ӷ�����

	bool shutdown;//Ҫ��Ҫ�����̳߳أ�����Ϊ1��������Ϊ0

};

