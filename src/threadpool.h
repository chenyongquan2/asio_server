#pragma once

#include <asio.hpp>
#include <cstddef>
#include <future>
#include "server.h"


constexpr size_t baseNum = 1; //10
class ThreadPool
{
public:
    template<typename ReturnType, typename Callable>
    std::future<ReturnType> AddTask(Callable&& callback);

private:
    
    size_t thread_num_{std::thread::hardware_concurrency() * baseNum}; 
    asio::thread_pool thread_pool_ {2};//构造初始化。
};

//模板实例化一般也在.h中，因为模板是在编译阶段去展开的。
template<typename ReturnType, typename Callable>
std::future<ReturnType> ThreadPool::AddTask(Callable&& callback)
{
    //这里的 () 表示无参数的函数调用
    std::packaged_task<ReturnType()> task(callback);
    std::future<ReturnType> future = task.get_future();

    asio::post(thread_pool_,[&task](){
        task();//无参数调用
    }
    );

    return future;
}