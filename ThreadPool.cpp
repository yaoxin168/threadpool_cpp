#include "ThreadPool.h"
#include <iostream>
#include <string>
#include <string.h>//memset
#include <unistd.h>//sleep
using namespace std;
//线程池初始化
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

		memset(threadIDs, 0, sizeof(pthread_t) * max);//把这片内存初始化为0，表示该位置未存储过线程id，是空着的
		minNum = min;
		maxNum = max;
		busyNum = 0;
		liveNum = min;//刚开始的存活线程数 和 最小线程个数相等
		exitNum = 0;

		//使用动态方法创建互斥锁和条件变量
		if (pthread_mutex_init(&mutexPool, NULL) != 0 ||
			pthread_cond_init(&notEmpty, NULL) != 0 )
		{
			cout << "mutex or condition fail" <<endl;
		}

		shutdown = false;

		//创建线程  //这里必须要传有个this，因为manager是静态成员函数，只能访问静态成员变量，这里可以通过this去访问这些变量
		pthread_create(&managerID, NULL, manager, this);//管理者线程
		//工作线程
		for (int i = 0; i < min; i++)
		{//工作线程的函数应该传入pool，到时候可以直接取到pool里面的任务队列taskQ和一些锁
			pthread_create(&threadIDs[i], NULL, worker, this);
		}

		return;


	} while (0);

	//初始化失败，需要释放资源
	if (threadIDs) delete[] threadIDs;
	if (taskQ) delete taskQ;

}


template<typename T>
ThreadPool<T>::~ThreadPool()
{
	//关闭线程池
	shutdown = true;
	//阻塞等待指定线程(管理者线程)结束才继续执行
	pthread_join(managerID, NULL);
	//唤醒阻塞的所有消费者线程(工作线程)，它们的代码处会判断线程池是否已关闭，如果已关闭，就自杀 
	for (int i = 0; i < liveNum; i++)
	{
		pthread_cond_signal(&notEmpty);
	}

	//释放堆内存
	if (taskQ) delete taskQ;
	if (threadIDs) delete[] threadIDs;

	//把所有互斥锁和条件变量资源释放掉
	pthread_mutex_destroy(&mutexPool);
	pthread_cond_destroy(&notEmpty);

}

template<typename T>
void ThreadPool<T>::addTask(Task<T> task)
{
	//线程池已被关闭
	if (shutdown)
	{
		pthread_mutex_unlock(&mutexPool);
		return;
	}
	//添加任务到任务队列的队尾
	taskQ->addTask(task);//函数里面加锁做了同步，这里外面调用就不用再加锁解锁了

	//现在任务队列上有任务了，需要唤醒工作线程
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

//工作线程的函数
template<typename T>
void* ThreadPool<T>::worker(void* arg)
{
	//1.首先对传进来的this进行强制类型转换，用于访问类的成员变量
	ThreadPool* pool = static_cast<ThreadPool*> (arg);
	//2.循环读取任务队列，而任务队列是共享资源，需要互斥访问
	while (1)
	{
		pthread_mutex_lock(&pool->mutexPool);//mutexPool拿来专门锁整个线程池
		//当前任务个数为0且线程池未被关闭
		while (pool->taskQ->taskNumber() == 0 && !pool->shutdown)
		{
			//那么应该阻塞工作线程
			pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);

			//判当前需不需要销毁线程
			if (pool->exitNum > 0)//在管理者发现闲着的线程过多时，会设置pool->exitNum值
			{
				//pthread_cond_wait在wait时自动释放锁，在被唤醒后又自动加锁了
				//为了避免死锁，这里需要在线程自杀前释放锁
				pool->exitNum--;//不管工作线程是否真的自杀，这里都应该--。否则，exitNum始终非0，所有被唤醒的线程都来自杀了
				if (pool->liveNum > pool->minNum)
				{
					pool->liveNum--;//马上线程自杀了，活着的线程数应该-1
					pthread_mutex_unlock(&pool->mutexPool);
					pool->threadExit();//线程自杀
				}

			}

		}
		//线程池已经关闭了
		if (pool->shutdown)
		{
			pthread_mutex_unlock(&pool->mutexPool);
			/*pthread_exit的好处：
			子线程使用pthread_exit退出后,内核会自动回收子线程相关的数据结构。
			父线程调用wait()时, 直接获取退出状态不需要进行任何清理操作。
			因此，避免了僵尸进程的产生*/
			pool->threadExit();//其中调用pthread_exit(NULL);
		}

		////////////当前是正常的情况，工作线程进行消费////////////
		//从任务队列的头部取出一个任务
		Task<T> task = pool->taskQ->takeTask();

		pool->busyNum++;
		pthread_mutex_unlock(&pool->mutexPool);

		//调用任务函数，那么现在又多了一个忙线程，busyNumber应该+1
		cout << "thread" << to_string(pthread_self())<<"start working..." << endl;
	
		task.function(task.arg);
		delete task.arg;//由于传的是一块堆内存，需要释放
		task.arg = nullptr;

		//任务函数执行结束，忙线程-1
		cout << "thread" << to_string(pthread_self()) << "end working..." << endl;
		pthread_mutex_lock(&pool->mutexPool);
		pool->busyNum--;
		pthread_mutex_unlock(&pool->mutexPool);

	}
	return nullptr;
}

//管理者线程：负责调节线程池中的线程个数
template<typename T>
void* ThreadPool<T>::manager(void* arg)
{
	ThreadPool* pool = static_cast<ThreadPool*>(arg);

	while (!pool->shutdown)//线程池只要没关闭，管理者就一直检测
	{
		//每隔3s检测1次
		sleep(3);
		//由于要取共享资源，即线程池相关变量的值，需要加锁
		pthread_mutex_lock(&pool->mutexPool);
		int queueSize = pool->taskQ->taskNumber();
		int liveNum = pool->liveNum;
		int busyNum = pool->busyNum;
		pthread_mutex_unlock(&pool->mutexPool);


		//添加线程：规定，当前任务个数>存活线程数 && 存活线程数<最大线程数，则添加线程
		if (queueSize > liveNum && liveNum < pool->maxNum)//maxNum是永远不会被改变的，因此无需互斥访问
		{
			pthread_mutex_lock(&pool->mutexPool);//因为要操作pool->liveNum

			int counter = 0;
			//从存储线程id的数组中找到空闲位置 来存储新创建的线程id
			for (int i = 0; i < pool->maxNum && counter < NUMBER && pool->liveNum < pool->maxNum; i++)
			{
				if (pool->threadIDs[i] == 0)//该位置的内存还没有存储线程id
				{
					pthread_create(&pool->threadIDs[i], NULL, worker, pool);//创建线程
					counter++;
					pool->liveNum++;
				}

			}

			pthread_mutex_unlock(&pool->mutexPool);
		}

		//销毁线程：规定，忙线程数*2<存活线程数 && 存活线程数>最小线程数
		if (busyNum * 2 < liveNum && liveNum > pool->minNum)
		{
			pthread_mutex_lock(&pool->mutexPool);
			pool->exitNum = NUMBER;
			pthread_mutex_unlock(&pool->mutexPool);

			//让工作线程自杀：唤醒wait的工作线程，工作线程会去检查pool->exitNum，非0则工作线程自杀
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
	pthread_t tid = pthread_self();//获取当前线程ID
	for (int i = 0; i < maxNum; i++)
	{
		if (threadIDs[i] == tid)
		{
			threadIDs[i] = 0;
			cout << "threadExit() called" << to_string(tid) << "exiting..." << endl;
			break;
		}
	}
	pthread_exit(NULL);//这里不要写成nullptr，因为这是标准c的函数，而不是c++的函数
}

