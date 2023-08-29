#include "ThreadPool.h"
#include <iostream>
#include <string>
#include <string.h>//memset
#include <unistd.h>//sleep
using namespace std;
//�̳߳س�ʼ��
template<typename T>
ThreadPool<T>::ThreadPool(int min, int max)
{
	
	do
	{
		taskQ = new TaskQueue<T>;
		if (taskQ == nullptr)
		{
			cout << "malloc taskQ fail..." << endl;
			break;
		}

		threadIDs = new pthread_t[max];
		if (threadIDs == nullptr)
		{
			cout << "malloc threadIDs fail..." << endl;
			break;
		}

		memset(threadIDs, 0, sizeof(pthread_t) * max);//����Ƭ�ڴ��ʼ��Ϊ0����ʾ��λ��δ�洢���߳�id���ǿ��ŵ�
		minNum = min;
		maxNum = max;
		busyNum = 0;
		liveNum = min;//�տ�ʼ�Ĵ���߳��� �� ��С�̸߳������
		exitNum = 0;

		//ʹ�ö�̬������������������������
		if (pthread_mutex_init(&mutexPool, NULL) != 0 ||
			pthread_cond_init(&notEmpty, NULL) != 0 )
		{
			cout << "mutex or condition fail" <<endl;
		}

		shutdown = false;

		//�����߳�  //�������Ҫ���и�this����Ϊmanager�Ǿ�̬��Ա������ֻ�ܷ��ʾ�̬��Ա�������������ͨ��thisȥ������Щ����
		pthread_create(&managerID, NULL, manager, this);//�������߳�
		//�����߳�
		for (int i = 0; i < min; i++)
		{//�����̵߳ĺ���Ӧ�ô���pool����ʱ�����ֱ��ȡ��pool������������taskQ��һЩ��
			pthread_create(&threadIDs[i], NULL, worker, this);
		}

		return;


	} while (0);

	//��ʼ��ʧ�ܣ���Ҫ�ͷ���Դ
	if (threadIDs) delete[] threadIDs;
	if (taskQ) delete taskQ;

}


template<typename T>
ThreadPool<T>::~ThreadPool()
{
	//�ر��̳߳�
	shutdown = true;
	//�����ȴ�ָ���߳�(�������߳�)�����ż���ִ��
	pthread_join(managerID, NULL);
	//���������������������߳�(�����߳�)�����ǵĴ��봦���ж��̳߳��Ƿ��ѹرգ�����ѹرգ�����ɱ 
	for (int i = 0; i < liveNum; i++)
	{
		pthread_cond_signal(&notEmpty);
	}

	//�ͷŶ��ڴ�
	if (taskQ) delete taskQ;
	if (threadIDs) delete[] threadIDs;

	//�����л�����������������Դ�ͷŵ�
	pthread_mutex_destroy(&mutexPool);
	pthread_cond_destroy(&notEmpty);

}

template<typename T>
void ThreadPool<T>::addTask(Task<T> task)
{
	//�̳߳��ѱ��ر�
	if (shutdown)
	{
		pthread_mutex_unlock(&mutexPool);
		return;
	}
	//�������������еĶ�β
	taskQ->addTask(task);//���������������ͬ��������������þͲ����ټ���������

	//��������������������ˣ���Ҫ���ѹ����߳�
	pthread_cond_signal(&notEmpty);

}

template<typename T>
int ThreadPool<T>::getBusyNum()
{
	pthread_mutex_lock(&mutexPool);
	int busyNum = this->busyNum;
	pthread_mutex_unlock(&mutexPool);
	return busyNum;
}

template<typename T>
int ThreadPool<T>::getAliveNum()
{
	pthread_mutex_lock(&mutexPool);
	int aliveNum = this->liveNum;
	pthread_mutex_unlock(&mutexPool);
	return aliveNum;
}

//�����̵߳ĺ���
template<typename T>
void* ThreadPool<T>::worker(void* arg)
{
	//1.���ȶԴ�������this����ǿ������ת�������ڷ�����ĳ�Ա����
	ThreadPool* pool = static_cast<ThreadPool*> (arg);
	//2.ѭ����ȡ������У�����������ǹ�����Դ����Ҫ�������
	while (1)
	{
		pthread_mutex_lock(&pool->mutexPool);//mutexPool����ר���������̳߳�
		//��ǰ�������Ϊ0���̳߳�δ���ر�
		while (pool->taskQ->taskNumber() == 0 && !pool->shutdown)
		{
			//��ôӦ�����������߳�
			pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);

			//�е�ǰ�費��Ҫ�����߳�
			if (pool->exitNum > 0)//�ڹ����߷������ŵ��̹߳���ʱ��������pool->exitNumֵ
			{
				//pthread_cond_wait��waitʱ�Զ��ͷ������ڱ����Ѻ����Զ�������
				//Ϊ�˱���������������Ҫ���߳���ɱǰ�ͷ���
				pool->exitNum--;//���ܹ����߳��Ƿ������ɱ�����ﶼӦ��--������exitNumʼ�շ�0�����б����ѵ��̶߳�����ɱ��
				if (pool->liveNum > pool->minNum)
				{
					pool->liveNum--;//�����߳���ɱ�ˣ����ŵ��߳���Ӧ��-1
					pthread_mutex_unlock(&pool->mutexPool);
					pool->threadExit();//�߳���ɱ
				}

			}

		}
		//�̳߳��Ѿ��ر���
		if (pool->shutdown)
		{
			pthread_mutex_unlock(&pool->mutexPool);
			/*pthread_exit�ĺô���
			���߳�ʹ��pthread_exit�˳���,�ں˻��Զ��������߳���ص����ݽṹ��
			���̵߳���wait()ʱ, ֱ�ӻ�ȡ�˳�״̬����Ҫ�����κ����������
			��ˣ������˽�ʬ���̵Ĳ���*/
			pool->threadExit();//���е���pthread_exit(NULL);
		}

		////////////��ǰ������������������߳̽�������////////////
		//��������е�ͷ��ȡ��һ������
		Task<T> task = pool->taskQ->takeTask();

		pool->busyNum++;
		pthread_mutex_unlock(&pool->mutexPool);

		//��������������ô�����ֶ���һ��æ�̣߳�busyNumberӦ��+1
		cout << "thread" << to_string(pthread_self())<<"start working..." << endl;
	
		task.function(task.arg);
		delete task.arg;//���ڴ�����һ����ڴ棬��Ҫ�ͷ�
		task.arg = nullptr;

		//������ִ�н�����æ�߳�-1
		cout << "thread" << to_string(pthread_self()) << "end working..." << endl;
		pthread_mutex_lock(&pool->mutexPool);
		pool->busyNum--;
		pthread_mutex_unlock(&pool->mutexPool);

	}
	return nullptr;
}

//�������̣߳���������̳߳��е��̸߳���
template<typename T>
void* ThreadPool<T>::manager(void* arg)
{
	ThreadPool* pool = static_cast<ThreadPool*>(arg);

	while (!pool->shutdown)//�̳߳�ֻҪû�رգ������߾�һֱ���
	{
		//ÿ��3s���1��
		sleep(3);
		//����Ҫȡ������Դ�����̳߳���ر�����ֵ����Ҫ����
		pthread_mutex_lock(&pool->mutexPool);
		int queueSize = pool->taskQ->taskNumber();
		int liveNum = pool->liveNum;
		int busyNum = pool->busyNum;
		pthread_mutex_unlock(&pool->mutexPool);


		//����̣߳��涨����ǰ�������>����߳��� && ����߳���<����߳�����������߳�
		if (queueSize > liveNum && liveNum < pool->maxNum)//maxNum����Զ���ᱻ�ı�ģ�������軥�����
		{
			pthread_mutex_lock(&pool->mutexPool);//��ΪҪ����pool->liveNum

			int counter = 0;
			//�Ӵ洢�߳�id���������ҵ�����λ�� ���洢�´������߳�id
			for (int i = 0; i < pool->maxNum && counter < NUMBER && pool->liveNum < pool->maxNum; i++)
			{
				if (pool->threadIDs[i] == 0)//��λ�õ��ڴ滹û�д洢�߳�id
				{
					pthread_create(&pool->threadIDs[i], NULL, worker, pool);//�����߳�
					counter++;
					pool->liveNum++;
				}

			}

			pthread_mutex_unlock(&pool->mutexPool);
		}

		//�����̣߳��涨��æ�߳���*2<����߳��� && ����߳���>��С�߳���
		if (busyNum * 2 < liveNum && liveNum > pool->minNum)
		{
			pthread_mutex_lock(&pool->mutexPool);
			pool->exitNum = NUMBER;
			pthread_mutex_unlock(&pool->mutexPool);

			//�ù����߳���ɱ������wait�Ĺ����̣߳������̻߳�ȥ���pool->exitNum����0�����߳���ɱ
			for (int i = 0; i < NUMBER; i++)
			{
				pthread_cond_signal(&pool->notEmpty);
			}
		}

	}
	return NULL;
}

template<typename T>
void ThreadPool<T>::threadExit()
{
	pthread_t tid = pthread_self();//��ȡ��ǰ�߳�ID
	for (int i = 0; i < maxNum; i++)
	{
		if (threadIDs[i] == tid)
		{
			threadIDs[i] = 0;
			cout << "threadExit() called" << to_string(tid) << "exiting..." << endl;
			break;
		}
	}
	pthread_exit(NULL);//���ﲻҪд��nullptr����Ϊ���Ǳ�׼c�ĺ�����������c++�ĺ���
}

