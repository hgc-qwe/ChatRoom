#include <iostream>
#include <unistd.h>

#include "EventLoopThread.h"
#include "EventLoop.h"


int main()
{

    std::cout 
        << "main thread start"
        << std::endl;


    EventLoopThread thread;


    // 启动子 Reactor线程
    EventLoop* loop = thread.startLoop();


    std::cout
        << "sub loop created"
        << std::endl;



    sleep(2);



    // 测试跨线程执行
    loop->runInLoop([](){

        std::cout
            << "hello from sub loop thread"
            << std::endl;

    });



    // 再测试一次
    sleep(1);


    loop->queueInLoop([](){

        std::cout
            << "queue task execute"
            << std::endl;

    });



    while(true)
    {
        sleep(1);
    }


    return 0;
}