
#include "server.h"
#include "stdafx.h"
#include "server.h"
#include <iostream>
using namespace std;

void my_task()
{
    std::this_thread::sleep_for(std::chrono::seconds(2));
    cout << "my_task"  <<endl;
}

int main()
{
    TcpServer server("0.0.0.0", 6688);
    server.Start();

    // asio::io_context io_context;
    // //提交一个函数
    // asio::post(io_context, my_task);
    
    // //提交一个lambda 表达式
    // asio::post(io_context, [](){
    //     cout << "post"  <<endl;
    //     std::this_thread::sleep_for(std::chrono::seconds(3));
    // });

    // //运行 io_context 直到它用完为止。
    // io_context.run();

    cout << "server exit" <<endl;
}