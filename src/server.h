#pragma once


#include <asio.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/signal_set.hpp>
#include <asio/write.hpp>

#include <cstddef>
#include <string>
#include <atomic>
#include <memory>

using namespace std;

class ThreadPool;
struct RequestContext;

class TcpServer
{
public:
    TcpServer(std::string ip_address, unsigned short port);
    void Start();

    static ThreadPool& GetThreadPool()
    {
        return threadpool_;
    }
    

private:
    void onSocketAccept(std::shared_ptr<asio::ip::tcp::socket> socket);
    void onSocketRecv(char* buf, std::shared_ptr<asio::ip::tcp::socket> socket);
    void onSocketSend(char* buf, std::shared_ptr<asio::ip::tcp::socket> socket);

private:
    asio::awaitable<void> Serve(std::shared_ptr<asio::ip::tcp::socket> socket);

    asio::awaitable<std::string> ReadMessage(std::shared_ptr<asio::ip::tcp::socket> socket);

    void TcpRequestHandle(RequestContext & request_context);
    void RequestTaskHandle(RequestContext request_context);
private:
    static asio::io_context io_context_;

    std::string ip_address_;
    unsigned short port_;

    static ThreadPool threadpool_;
};
