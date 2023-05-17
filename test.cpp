#include <iostream>
#include <stdexcept>
#include "ThreadPool.h"
#include <unistd.h>


using namespace std;

class testclass{
private:
    int num;
public:
    testclass(int a){
        num = a;
    }
    ~testclass(){}
    void process(){
        for(int i = 0; i < 50; i++){
            cout << "task is " + to_string(num) << endl;
            sleep(0.1);
        }   
        
    }
};

int main(){
    try{
        ThreadPool<testclass>* tp = new ThreadPool<testclass>();

        tp->createThreads();
        testclass c1(1);
        testclass c2(2);
        testclass c3(3);
        tp->addTask(&c1);
        tp->addTask(&c2);
        tp->addTask(&c3);
        sleep(12);
        delete tp;
        //threadpool<testclass> tp;
    }
    catch(exception){
        cout << "error" << endl;
    }
}