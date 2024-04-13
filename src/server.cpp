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
    asio::ip::tcp::acceptor acceptor(io_context_);
    asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), port_);

    std::error_code ec;
    acceptor.open(endpoint.protocol(), ec);
    if (ec)
        return;
    acceptor.bind(endpoint, ec);

    // if(!ec)
    // {
    //     SPDLOG_ERROR("bind fail");
    // }
    //acceptor.close();

    //开启一个协程去进行listen
    co_spawn(io_context_, Listener(), detached);

    //启动事件循环，以此来消费io事件
    //std::this_thread::sleep_for(std::chrono::seconds(3));
    //io_context_.run();

    GetThreadPool().AddTask<void>([&, this]() {
            try {
                asio::signal_set signals(io_context_, SIGINT, SIGTERM);
                signals.async_wait([&](auto, auto) { io_context_.stop(); });    
                io_context_.run();//启动一个 I/O 上下文的事件循环。不断的去事件循环，消费io事件。
                std::cout << "io_context_.run() finish"  << std::endl;
            } catch (std::exception& e) {
                SPDLOG_ERROR("TcpServer Start exception: {}", e.what());
            }
    }
    );
    //std::this_thread::sleep_for(std::chrono::seconds(3));
    io_context_.run();
}

asio::awaitable<void> TcpServer::Listener()
{
    tcp::acceptor acceptor(io_context_, { asio::ip::make_address(ip_address_), port_ });
    // asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), port_);
    // tcp::acceptor acceptor(io_context_, endpoint);
    for(;;)//不断阻塞去等待新连接
    {
        auto socket = std::make_shared<tcp::socket>(io_context_);
        co_await acceptor.async_accept(*socket, use_awaitable);

        //开启一个新的协程去让这个socket去处理业务，防止把当前负责accept的线程给阻塞
        //绑定io_context
        //使用 co_spawn 函数将 Serve 函数作为一个协程任务提交给 io_context_
        co_spawn(io_context_,Serve(socket),detached);
    }
    co_return;
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
