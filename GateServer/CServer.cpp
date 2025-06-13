#include "CServer.h"
#include "AsioIOServerPool.h"

CServer::CServer(net::io_context& ioc, unsigned short& port)
	:_ioc(ioc), _acceptor(ioc, tcp::endpoint(tcp::v4(), port)) {

}


void CServer::start() {
	auto self = shared_from_this();

	//异步接收客户端连接,引入连接池提高并发能力
	auto& serv = AsioIOServerPool::getInstance()->getIOContext();
	std::shared_ptr<HttpConnection> connect = std::make_shared<HttpConnection>(*serv);

	_acceptor.async_accept(connect->getSocket(), [self,connect](beast::error_code e) {
		try {
			if (e) {  //如果错误码不为0->出错，则抛弃这个连接，继续监听下一个连接
				self->start();
				return;
			}
			//创建一个 会话类 处理这个连接
			connect->start();

			self->start();
		}
		catch (std::exception& ec) {
			std::cout << "ecception is :" << ec.what() << std::endl;
		}
		});
}
