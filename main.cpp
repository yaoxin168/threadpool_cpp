#include "ThreadPool.h"
#include "ThreadPool.cpp"//对于模板类，只包含头文件是不够的，除非类的声明和定义都写在头文件里了
#include <pthread.h>//pthread_self
#include <unistd.h>//sleep
#include <stdlib.h>//malloc
#include <iostream>
using namespace std;

//工作函数
void taskFunc(void* arg)
{
    int num = *(int*)arg;
    cout << "thread " << to_string(pthread_self()) << "is working, number = " << to_string(num) << endl;
    sleep(1);
}


int main()
{
    //创建线程池
    ThreadPool<int> pool(3, 10);
    //添加任务
    for (int i = 0; i < 100; i++)
    {
        int* num = new int(i+100);//使用new创建并初始化为i+100
        pool.addTask(Task<int>(taskFunc, num));
    }

    sleep(30);

    return 0;
}