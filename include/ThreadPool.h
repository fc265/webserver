#ifndef THREADPOOL_H
#define THREADPOOL_H

#define MAX_THREAD_NUM 10

#include <iostream>
#include <queue>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>


using namespace std;

template<class T>
class ThreadPool{
private:
    static queue<T*> tasks;
    static vector<pthread_t* > threads; 
    static void *worker(void * arg);
    static pthread_mutex_t mutex;
    static sem_t sem;
    static sem_t sem_task;
    static bool m_stop;
public:
    ThreadPool();
    ~ThreadPool();
    bool addTask(T* task);
    void createThreads();
    int getTaskNum();
};

template<class T>
queue<T*> ThreadPool<T>::tasks;

template<class T>
vector<pthread_t* > ThreadPool<T>::threads;

template<class T>
pthread_mutex_t ThreadPool<T>::mutex;

template<class T>
sem_t ThreadPool<T>::sem;

template<class T>
sem_t ThreadPool<T>::sem_task;

template<class T>
bool ThreadPool<T>::m_stop = false;

//构造函数
template <class T>
ThreadPool<T>::ThreadPool(){
    pthread_mutex_init(&mutex, NULL);
    sem_init(&sem, 0, MAX_THREAD_NUM);
    sem_init(&sem_task, 0, 0);
    cout << "construct" << endl;
}

//析构函数
template <class T>
ThreadPool<T>::~ThreadPool(){
    pthread_mutex_destroy(&mutex);
    sem_destroy(&sem);
    sem_destroy(&sem_task);
    cout << "destroy" << endl;
}

//添加任务
template <class T>
bool ThreadPool<T>::addTask(T* task){
    pthread_mutex_lock(&mutex);
    tasks.push(task);
    sem_post(&sem_task);
    pthread_mutex_unlock(&mutex);
    return true;
}


//初始化线程池队列
template <class T>
void ThreadPool<T>::createThreads(){
    for(int i = 0; i < MAX_THREAD_NUM; i++){
        pthread_t thread;
        int t = pthread_create(&thread, NULL, worker, NULL);
        sleep(0.5);
        if(t == 0) threads.push_back(&thread);
        else{
            cout << "create thread error" << endl;
            throw exception();
        }
        if(pthread_detach(*threads.back())){
            cout << "detach error" << endl;
            throw exception();
        }
        
    }
}


template <class T>
void* ThreadPool<T>::worker(void * arg){
    while(!m_stop){
        sem_wait(&sem_task);
        sem_wait(&sem);
        pthread_mutex_lock(&mutex);
        T* task = tasks.front();
        tasks.pop();
        pthread_mutex_unlock(&mutex);
        task->process();
        sem_post(&sem);
    }
    return arg;
}


template <class T>
int ThreadPool<T>::getTaskNum(){
    return tasks.size();
}

#endif