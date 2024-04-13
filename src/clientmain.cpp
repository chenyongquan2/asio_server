#include<iostream>
#include <asio.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/signal_set.hpp>
#include <asio/write.hpp>
#include "asio/detached.hpp"


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

int main() {
	asio::io_context io;
	asio::ip::tcp::socket sock(io);
	sock.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"),6688));

	char buf[0xFF];
	while (true) {
		cin >> buf;
		sock.send(asio::buffer(buf));
		memset(buf, 0, 0xFF);
		sock.receive(buffer(buf));
		cout << buf<<endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
	}
	sock.close();
	::system("pause");
}