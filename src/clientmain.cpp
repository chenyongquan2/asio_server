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

std::string getauthorizationid() {
    return "cyq";
}

std::string getusername() {
    return "cyq";
}

std::string getsimple() {
    // 这里您需要提供客户端用户的密码
    return "cyq";
}

std::string getrealm() {
    return "CPPTEAM.DOO.COM";
}


int main() {

	sasl_conn_t *conn = nullptr;
	{
		// 初始化 SASL 环境
		int rc;
		//sasl_conn_t* conn;
		sasl_callback_t client_callbacks[] = {
			{
				.id = SASL_CB_AUTHNAME,
				.proc = (int (*)(void))getauthorizationid,
				.context = NULL
			},
			{
				.id = SASL_CB_USER,
				.proc = (int (*)(void))getusername,
				.context = NULL
			},
			{
				.id = SASL_CB_PASS,
				.proc = (int (*)(void))getsimple,
				.context = NULL
			},
			{
				.id = SASL_CB_GETREALM,
				.proc = (int (*)(void))getrealm,
				.context = NULL
			},
			{
				.id = SASL_CB_LIST_END,
				.proc = NULL,
				.context = NULL
			}
		};
		rc = sasl_client_init(client_callbacks);
		SPDLOG_INFO("sasl_client_init result: {}", rc);
		if (rc != SASL_OK) {
			SPDLOG_ERROR("Failed to initialize SASL: {}", rc);
			return 1;
		}

		

		// 创建 SASL 连接
		//rc = sasl_client_new("cyq", "CPPTEAM.DOO.COM", NULL, NULL, client_callbacks, 0, &conn);
		rc = sasl_client_new("shaq", "127.0.0.1", "10.1.14.41", NULL, client_callbacks, 0, &conn);
		SPDLOG_INFO("sasl_client_new result: {}", rc);
		if (rc != SASL_OK) {
			SPDLOG_ERROR("Failed to create SASL connection: {}", rc);
			return 1;
		}

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
				return 1;
			}

			// 搜索 result 字符串中是否包含 "GSSAPI"
			tmp = result;
			while ((tmp = strstr(tmp, "GSSAPI")) != NULL) {
				// GSSAPI 机制可用
				SPDLOG_INFO("GSSAPI mechanism is supported");
				break;
			}

			if (tmp == NULL) {
				// GSSAPI 机制不可用
				SPDLOG_ERROR("GSSAPI mechanism is not supported");
			}
		}

		// 设置 SASL 机制为 GSSAPI
		rc = sasl_setprop(conn, SASL_AUTH_EXTERNAL, "GSSAPI");
		SPDLOG_INFO("sasl_setprop result: {}", rc);
		if (rc != SASL_OK) {
			SPDLOG_ERROR("Failed to sasl_setprop SASL: {}", rc);
			return 1;
		}
	}

	asio::io_context io;
	asio::ip::tcp::socket sock(io);
	sock.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"),6688));

	{
		// 进行 SASL 身份验证
		const char *data = NULL;
		unsigned len = 0;
		int result = 0;
		//const char *mechlist = "GSSAPI SCRAM-SHA-1 DIGEST-MD5 EXTERNAL NTLM LOGIN PLAIN ANONYMOUS";
		const char *mechlist = "GSSAPI";
		const char * mech = nullptr;
		
		do {
			//result = sasl_client_start(conn, "GSSAPI", NULL, &data, &len, NULL);
			SPDLOG_INFO("sasl_client_start begin!!!");
			result = sals_client_start(conn, mechlist, NULL, &data, &len, &mech);
			SPDLOG_INFO("sasl_client_start result: {}, mech:{}", result, mech);
			if (result==SASL_INTERACT)
			{
				/* [deal with the interactions. See interactions section below] */
				SPDLOG_INFO("sasl_client_start result: {}, deal with the interactions. See interactions section below", result);
			}
		} while (result==SASL_INTERACT || result == SASL_CONTINUE);

		if (result!=SASL_OK) /* [failure] */
		{
			SPDLOG_ERROR("sasl_client_start failed!result: {}", result);
		}
		else
		{
			SPDLOG_INFO("sasl_client_start ok!result: {}", result);
		}

		//agin 
		result = sasl_client_start(conn, "GSSAPI", NULL, &data, &len, NULL);
		if (result!=SASL_OK) /* [failure] */
		{
			SPDLOG_ERROR("[again]sasl_client_start failed!result: {}", result);
		}
		else
		{
			SPDLOG_INFO("[again]sasl_client_start ok!result: {}", result);
		}
	}

	char buf[0xFF];
	while (true) {
		cin >> buf;
		sock.send(asio::buffer(buf));
		memset(buf, 0, 0xFF);
		sock.receive(buffer(buf));
		SPDLOG_INFO("recv msg {}", buf);

        std::this_thread::sleep_for(std::chrono::seconds(2));
	}
	sock.close();
	::system("pause");
}