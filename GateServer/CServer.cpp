#include "CServer.h"
#include "AsioIOServerPool.h"

CServer::CServer(net::io_context& ioc, unsigned short& port)
	:_ioc(ioc), _acceptor(ioc, tcp::endpoint(tcp::v4(), port)) {

}


void CServer::start() {
	auto self = shared_from_this();

	//�첽���տͻ�������,�������ӳ���߲�������
	auto& serv = AsioIOServerPool::getInstance()->getIOContext();
	std::shared_ptr<HttpConnection> connect = std::make_shared<HttpConnection>(*serv);

	_acceptor.async_accept(connect->getSocket(), [self,connect](beast::error_code e) {
		try {
			if (e) {  //��������벻Ϊ0->����������������ӣ�����������һ������
				self->start();
				return;
			}
			//����һ�� �Ự�� �����������
			connect->start();

			self->start();
		}
		catch (std::exception& ec) {
			std::cout << "ecception is :" << ec.what() << std::endl;
		}
		});
}
