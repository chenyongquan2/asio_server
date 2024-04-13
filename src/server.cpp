#include "stdafx.h"

#include <cstddef>
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <random>
#include <iostream>

#include <asio.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/signal_set.hpp>
#include <asio/write.hpp>
#include "asio/detached.hpp"

#include "threadpool.h"
#include "requestcontext.h"
#include "server.h"

using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::use_awaitable;
using asio::ip::tcp;
namespace this_coro = asio::this_coro;

using namespace std;

//静态成员初始化
ThreadPool TcpServer::threadpool_;
asio::io_context TcpServer::io_context_;

TcpServer::TcpServer(std::string ip_address, unsigned short port)
    :ip_address_(ip_address)
    ,port_(port)
{

}

void TcpServer::Start()
{
    asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), port_);
    asio::ip::tcp::acceptor acptr(io_context_, endpoint);

    auto socket = std::make_shared<tcp::socket>(io_context_);

    //async异步的情况下，程序不会卡在async_accept这里，它仅仅只是提交了一个接受客户端连接的请求，等待系统执行完成后，调用对应的处理函数就行了
    
    //这个bind函数，其主要作用就是更改函数特性，
    //比如我这里，第一个参数为要更改的函数，第二参数是我想要传给这个函数的参数，
    //bind函数接收到这两个参数，就会返回一个无参函数，就可以符合async_accept第二个参数的要求。
    //每当程序调用这个返回的无参函数时，都会将这个sock变量传入实际的sock_accept函数进行调用
    acptr.async_accept(*socket, std::bind(&TcpServer::onSocketAccept, this, socket));

    //而run函数，只要还有一个请求没有完成，它就不会返回。
    io_context_.run();
}

void TcpServer::onSocketAccept(std::shared_ptr<asio::ip::tcp::socket> sockCli)
{
    char* buf = new char[0xFF];
    SPDLOG_INFO("new conn from {}:{}", sockCli->remote_endpoint().address().to_string() ,sockCli->remote_endpoint().port());
	std::cout << "have accepted client , ip:" << sockCli->remote_endpoint().address() 
        << ",port:" << sockCli->remote_endpoint().port() << std::endl;
	
    //1缓存区都需要通过buffer函数来构造函数可接受的缓存区结构体，否则会报错误
    //buffer函数，它的作用其实就是构造一个结构体,大致如下：
    // struct{
    //     void* buf;
    //     s_size len;
    // }
    //该网络模块中所有的收发数据操作，都不接受单独的字符串，而是这样一个结构体，分别为缓存区的首地址以及缓存区的大小。

    sockCli->async_receive(asio::buffer(buf, 0xFF), std::bind(&TcpServer::onSocketRecv, this, buf, sockCli));
}

void TcpServer::onSocketRecv(char* buf, std::shared_ptr<asio::ip::tcp::socket> sockCli)
{
    try
    {
        std::cout << "server recv msg:" << buf << endl;
        sockCli->async_send(asio::buffer(buf,0xFF), std::bind(&TcpServer::onSocketSend, this, buf, sockCli));
    }
    catch (const std::exception& e)
    {
        cout << "";
		cout << e.what();
		delete[] buf;
    }
}

void TcpServer::onSocketSend(char* buf, std::shared_ptr<asio::ip::tcp::socket> sockCli)
{
    try {
        std::cout << "server send msg:" << buf << endl;
		sockCli->async_receive(asio::buffer(buf, 0xFF), std::bind(&TcpServer::onSocketRecv, this, buf, sockCli));
	}
	catch (std::exception& e) {
		cout << "";
		cout << e.what();
		delete[] buf;
	}
}

awaitable<void> TcpServer::Serve(std::shared_ptr<tcp::socket> socket)
{
    //conn连接上来后，会走到这里
    asio::ip::tcp::endpoint endpoint = socket->remote_endpoint();
    SPDLOG_INFO("new conn from {}:{}", endpoint.address().to_string() ,endpoint.port());

    //读数据
    while(1)
    {
        static size_t taskId = 0;
        taskId++;

        std::string message = co_await ReadMessage(socket);
        RequestContext request_context(taskId, message);
        TcpRequestHandle(request_context);
    }

    co_return;
}

asio::awaitable<std::string> TcpServer::ReadMessage(std::shared_ptr<tcp::socket> socket)
{
    std::string message;
    try 
    {
        //co_await asio::async_read_some(*socket_, asio::dynamic_buffer(message, 4096), asio::use_awaitable);
        size_t n = co_await socket->async_read_some(asio::buffer(message, 4096), asio::use_awaitable);
    
        SPDLOG_INFO("asio::async_read_some has read msg: {}, size: {}", message, n);
    }
    catch(std::exception& e)
    {
        SPDLOG_ERROR("asio::async_read_some Exception: {}, already read message: {}", e.what(), message);
    }
    co_return message;
    
}

void TcpServer::TcpRequestHandle(RequestContext & request_context)
{
    SPDLOG_INFO("TcpRequestHandle taskId:{}, msg: {}", request_context.task_id_, request_context.request_data_);

    //把任务放到线程池去处理
    auto task = [this, request_context = std::move(request_context)]()
    {
        RequestTaskHandle(request_context);
    };
    GetThreadPool().AddTask<void>(task);
}

void TcpServer::RequestTaskHandle(RequestContext request_context)
{
    std::random_device rd; // 获取随机数种子
    std::mt19937 eng(rd()); // 使用随机数种子初始化随机数引擎
    std::uniform_int_distribution<int> dist(1, 3); // 定义整数分布范围为1到3
    int random_number = dist(eng); // 生成随机数

    SPDLOG_INFO("TcpRequestHandle taskId:{}, willCost:{} sec", request_context.task_id_, random_number);

    std::this_thread::sleep_for(std::chrono::seconds(random_number));

    SPDLOG_INFO("TcpRequestHandle taskId:{}, hasCost:{} sec, finish deal with msg:{}", 
        request_context.task_id_, random_number, request_context.request_data_);

}
