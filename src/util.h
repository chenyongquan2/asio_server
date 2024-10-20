#pragma once

#include <asio.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/signal_set.hpp>
#include <asio/write.hpp>
#include <stdlib.h>
#include <string.h>>

#include <cstddef>
#include <string>
#include <atomic>
#include <memory>
#include <sasl/sasl.h>

int getpath(void *context, const char **path);
void recv_string(std::shared_ptr<asio::ip::tcp::socket> socket, char* buf, unsigned int* buflen, bool allow_eof);
void send_string(std::shared_ptr<asio::ip::tcp::socket> socket, const char* s, unsigned int l);

bool is_support_gssapi(sasl_conn_t *conn);