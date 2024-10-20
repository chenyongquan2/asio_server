#include "util.h"

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

using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::use_awaitable;
using asio::ip::tcp;
namespace this_coro = asio::this_coro;

#define PLUGINDIR "make_me_a_function_to_get_that_info"

int getpath(void *context, const char **path)
{
    if (!path) {
        return SASL_BADPARAM;
    }

    *path = PLUGINDIR;
    return SASL_OK;
}


void recv_string(std::shared_ptr<asio::ip::tcp::socket> socket, char* buf, unsigned int* buflen, bool allow_eof)
{
    unsigned int bufsize = *buflen;
    unsigned int l;

    *buflen = 0;

    // 读取长度
    asio::read(*socket, asio::buffer(&l, sizeof(l)), asio::transfer_all());
    if (allow_eof && l == 0) return;
    if (l > bufsize) throw std::length_error("Received data exceeds buffer size");

    // 读取数据
    asio::read(*socket, asio::buffer(buf, l), asio::transfer_all());
    *buflen = l;
}

void send_string(std::shared_ptr<asio::ip::tcp::socket> socket, const char* s, unsigned int l)
{
    try
    {
        // 发送数据长度
        unsigned int len = l;
        asio::write(*socket, asio::buffer(&len, sizeof(len)), asio::transfer_all());

        // 发送数据
        asio::write(*socket, asio::buffer(s, l), asio::transfer_all());
    }
    catch (std::exception& e)
    {
        SPDLOG_ERROR("Exception in send_string:{}",  e.what());
        throw;
    }
}

bool is_support_gssapi(sasl_conn_t *conn)
{
    // 检查是否支持 GSSAPI 机制
    const char *result, *tmp;
    unsigned int len;
    int ret;
    ret  = sasl_listmech(conn, NULL, "", ",", "", &result, &len, NULL);
    SPDLOG_INFO("sasl_listmech result: {}, listmech:{}", ret, result);
    if (ret != SASL_OK) {
        SPDLOG_INFO("sasl_listmech failed result: {}", ret);
        // 处理获取可用机制列表失败的情况
        return false;
    }

    // 搜索 result 字符串中是否包含 "GSSAPI"
    tmp = result;
    while ((tmp = strstr(tmp, "GSSAPI")) != NULL) {
        // GSSAPI 机制可用
        SPDLOG_INFO("GSSAPI mechanism is supported");
        return true;
        break;
    }

    if (tmp == NULL) {
        // GSSAPI 机制不可用
        SPDLOG_ERROR("GSSAPI mechanism is not supported");
        return false;
    }
    return true;
}