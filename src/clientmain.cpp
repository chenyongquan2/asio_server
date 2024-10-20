#include <cstddef>
#include <iostream>
#include <spdlog/spdlog.h>
#include <fmt/format.h>

#include <asio.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/signal_set.hpp>
#include <asio/write.hpp>
#include "asio/detached.hpp"
#include <sasl/sasl.h>
#include "util.h"

using namespace std;
using namespace asio;

// using 关键字用于引入 Boost.Asio 库中的命名空间和类型别名。具体来说：

// using asio::awaitable;：引入 asio 命名空间中的 awaitable 类型，使得可以直接使用 awaitable 而无需完整的命名空间限定符。

using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::use_awaitable;
using asio::ip::tcp;
namespace this_coro = asio::this_coro;

const char *sasl_mech = "GSSAPI";

// std::string getauthorizationid() {
//     return "cyq";
// }

// std::string getusername() {
//     return "cyq";
// }

// std::string getsimple() {
//     // 这里您需要提供客户端用户的密码
//     return "cyq";
// }

// std::string getrealm() {
//     return "CPPTEAM.DOO.COM";
// }

// static int get_user(void *context __attribute__((unused)),
//                   int id,
//                   const char **result,
//                   unsigned *len)
// {
//     const char *testuser = "test@CPPTEAM.DOO.COM";

//     if (! result)
//         return SASL_BADPARAM;

//     switch (id) {
//     case SASL_CB_USER:
//     case SASL_CB_AUTHNAME:
//         *result = testuser;
//         break;
//     default:
//         return SASL_BADPARAM;
//     }

//     if (len) *len = strlen(*result);

//     return SASL_OK;
// }

// const char *testpass = NULL;
// const char *testhost = NULL;

// static int get_pass(sasl_conn_t *conn __attribute__((unused)),
//           void *context __attribute__((unused)),
//           int id,
//           sasl_secret_t **psecret)
// {
//     size_t len;
//     static sasl_secret_t *x;

//     /* paranoia check */
//     if (! conn || ! psecret || id != SASL_CB_PASS)
//         return SASL_BADPARAM;

//     len = strlen(testpass);

//     x = (sasl_secret_t *) realloc(x, sizeof(sasl_secret_t) + len);

//     if (!x) {
//         return SASL_NOMEM;
//     }

//     x->len = len;
//     strcpy((char *)x->data, testpass);

//     *psecret = x;
//     return SASL_OK;
// }

int main() {
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


	sasl_conn_t *conn = nullptr;
	// 初始化 SASL 环境
	int rc = sasl_client_init(callbacks);
	SPDLOG_INFO("sasl_client_init result: {}", rc);
	if (rc != SASL_OK) {
		SPDLOG_ERROR("Failed to initialize SASL: {}", rc);
		return 1;
	}

	// 创建 SASL 连接
	rc = sasl_client_new("test", "CPPTEAM.DOO.COM", NULL, NULL, NULL, 0, &conn);
	SPDLOG_INFO("sasl_client_new result: {}", rc);
	if (rc != SASL_OK) {
		SPDLOG_ERROR("Failed to create SASL connection: {}", rc);
		return 1;
	}

	//测试是否允许gssapi
    is_support_gssapi(conn);

	sasl_channel_binding_t cb = {0};
	if (cb.name) {
		sasl_setprop(conn, SASL_CHANNEL_BINDING, &cb);
	}

	const char *data;
	unsigned int len;
	const char *chosenmech = NULL;

	rc = sasl_client_start(conn, sasl_mech, NULL, &data, &len, &chosenmech);
	SPDLOG_INFO("sasl_client_start result: {}, chosenmech:{}", rc, chosenmech);
	if (rc != SASL_OK && rc != SASL_CONTINUE) {
		SPDLOG_ERROR("Failed to sasl_client_start result: {}", rc);
		return 1;
	}

	asio::io_context io;
	auto sock = std::make_shared<tcp::socket>(io);
	sock->connect(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"),6688));

	char buf[8192];
	while (rc == SASL_CONTINUE) {
		send_string(sock, data, len);
		len = 8192;
		recv_string(sock, buf, &len, false);

		rc = sasl_client_step(conn, buf, len, NULL, &data, &len);
		if (rc != SASL_OK && rc != SASL_CONTINUE) {
			SPDLOG_ERROR("Failed to sasl_client_step result: {}", rc);
			return 1;
		}
	}

	if (rc != SASL_OK) 
	{
		SPDLOG_ERROR("Failed to sasl result: {}", rc);
		return 1;
	}

	if (len > 0) {
		send_string(sock, data, len);
	}

	SPDLOG_INFO("client Auth Done!!!");
	
	{
		char buf[0xFF];
		while (true) {
			cin >> buf;
			sock->send(asio::buffer(buf));
			memset(buf, 0, 0xFF);
			sock->receive(buffer(buf));
			SPDLOG_INFO("recv msg {}", buf);

			std::this_thread::sleep_for(std::chrono::seconds(2));
		}
		sock->close();
		::system("pause");
	}

	return 0;
}