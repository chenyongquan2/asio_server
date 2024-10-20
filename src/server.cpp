#include "stdafx.h"

#include <cstddef>
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <random>

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
#include "util.h"

using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::use_awaitable;
using asio::ip::tcp;
namespace this_coro = asio::this_coro;

const char *sasl_mech = "GSSAPI";


//静态成员初始化
ThreadPool TcpServer::threadpool_;
asio::io_context TcpServer::io_context_;

TcpServer::TcpServer(std::string ip_address, unsigned short port)
    :ip_address_(ip_address)
    ,port_(port)
{

}

// std::string getauthorizationid() {
//     return "test";
// }

// std::string getusername() {
//     return "test";
// }

// std::string getsimple() {
//     // 这里您需要提供客户端用户的密码
//     return "test";
// }

// std::string getrealm() {
//     return "CPPTEAM.DOO.COM";
// }

int TcpServer::saslServerInit()
{
    // 初始化 SASL 环境
    int rc;
    //sasl_conn_t* conn;
    sasl_callback_t callbacks[4] = {};
    /* initialize the sasl library */
    callbacks[0].id = SASL_CB_GETPATH;
    callbacks[0].proc = (int (*)(void))&getpath;
    callbacks[0].context = NULL;
    callbacks[1].id = SASL_CB_LIST_END;
    callbacks[1].proc = NULL;
    callbacks[1].context = NULL;
    callbacks[2].id = SASL_CB_LIST_END;
    callbacks[2].proc = NULL;
    callbacks[2].context = NULL;
    callbacks[3].id = SASL_CB_LIST_END;
    callbacks[3].proc = NULL;
    callbacks[3].context = NULL;

    rc = sasl_server_init(callbacks, "server");
    if (rc != SASL_OK) {
        SPDLOG_ERROR("Failed to initialize SASL: {}", rc);
        return 1;
    }
    // 创建 SASL 连接
    rc = sasl_server_new("test", "CPPTEAM.DOO.COM", NULL, NULL, NULL, callbacks, 0, &conn_);
    SPDLOG_INFO("sasl_server_new result: {}", rc);
    if (rc != SASL_OK) {
        SPDLOG_ERROR("Failed to create SASL connection: {}", rc);
        return 1;
    }

    //测试是否允许gssapi
    is_support_gssapi(conn_);

    sasl_channel_binding_t cb = {0};
    if (cb.name) {
        sasl_setprop(conn_, SASL_CHANNEL_BINDING, &cb);
    }

    return 0;

    // {
    //     //sasl_server_new("myservice", "localhost", NULL, NULL, NULL, callbacks, 0, &conn);
    //     // 设置 SASL 机制为 GSSAPI

    //     // #define SASL_SSF_EXTERNAL  100	/* external SSF active (sasl_ssf_t *) */
    //     // #define SASL_SEC_PROPS     101	/* sasl_security_properties_t */
    //     // #define SASL_AUTH_EXTERNAL 102	/* external authentication ID (const char *) */

    //     //const char *mechlist = "KERBEROS_V4,SCRAM-SHA-1,DIGEST-MD5,NTLM,LOGIN,PLAIN";
    //     //const char *mechlist = "GSSAPI,SCRAM-SHA-1,DIGEST-MD5,NTLM,LOGIN,PLAIN,ANONYMOUS";
    //     //rc = sasl_setprop(conn_, SASL_AUTH_EXTERNAL, mechlist);
    //     rc = sasl_setprop(conn_, SASL_AUTH_EXTERNAL, "GSSAPI");
    //     SPDLOG_INFO("sasl_setprop result: {}", rc);
    // }
}


int TcpServer::saslServerStart(std::shared_ptr<asio::ip::tcp::socket> sockCli)
{
    char buf[8192];
    unsigned int len = sizeof(buf);
    recv_string(sockCli, buf, &len, false);

    SPDLOG_INFO("Received data::{}", buf);
    const char *data;

    // 进行 SASL 身份验证
    int result = sasl_server_start(conn_, sasl_mech, buf, len, &data, &len);
    SPDLOG_INFO("sasl_server_start result:{}", result);
    if (result != SASL_OK && result != SASL_CONTINUE) {
        SPDLOG_ERROR("sasl_server_start failed!");
        return -1;
    }

    while (result == SASL_CONTINUE) {
        send_string(sockCli, data, len);
        len = 8192;
        recv_string(sockCli, buf, &len, true);

        result = sasl_server_step(conn_, buf, len, &data, &len);
        if (result != SASL_OK && result != SASL_CONTINUE) {
            SPDLOG_ERROR("sasl_server_start failed!");
            return -1;
        }
    }

    if (result != SASL_OK) 
    {
        SPDLOG_ERROR("sasl_server_start failed!");
        return -1;
    }

    if (len > 0) {
        send_string(sockCli, data, len);
    }

    SPDLOG_INFO("server Auth Done!!!");

    // if (result==SASL_OK)
    // {
    //     /* authentication succeeded. Send client the protocol specific message
    //     to say that authentication is complete */
    //     SPDLOG_INFO("SASL authentication successful!");
    // }
    // else if(result==SASL_CONTINUE)
    // {
    //     //retry again!
    //     // 继续进行 SASL 身份验证过程
    //     while (result == SASL_CONTINUE) {
    //         SPDLOG_INFO("SASL authentication in progress...");
    //         result = sasl_server_step(conn_, data, len, &data, &len);
    //     }
    //     if (result == SASL_OK) {
    //         SPDLOG_INFO("SASL authentication successful!");
    //     } else {
    //         SPDLOG_ERROR("SASL authentication failed!, result:{}",result);
    //         return result;
    //     }
    // }
    // else
    // {
    //     /* failure. Send protocol specific message that says authentication failed */
    //     SPDLOG_ERROR("SASL authentication failed!, result:{}",result);
    //     return result;
    // }
    // return result;
}

void TcpServer::Start()
{
    int res = saslServerInit();
    if(res != SASL_OK)
    {
        SPDLOG_ERROR("saslServerInit failed result: {}", res);
        return;
    }
    else
    {
        SPDLOG_INFO("saslServerInit success result: {}", res);
    }

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

    int res = saslServerStart(sockCli);
    if(res != SASL_OK)
    {
        SPDLOG_ERROR("saslServerStart failed result: {}", res);
        return;
    }
    else
    {
        SPDLOG_INFO("saslServerStart success result: {}", res);
    }

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
        SPDLOG_INFO("recv msg {}", buf);
        sockCli->async_send(asio::buffer(buf,0xFF), std::bind(&TcpServer::onSocketSend, this, buf, sockCli));
    }
    catch (const std::exception& e)
    {
        SPDLOG_INFO("exception occured: {}", e.what());
		delete[] buf;
    }
}

void TcpServer::onSocketSend(char* buf, std::shared_ptr<asio::ip::tcp::socket> sockCli)
{
    try {
        SPDLOG_INFO("send msg {}", buf);
		sockCli->async_receive(asio::buffer(buf, 0xFF), std::bind(&TcpServer::onSocketRecv, this, buf, sockCli));
	}
	catch (std::exception& e) {
		SPDLOG_ERROR("exception occured: {}", e.what());
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
