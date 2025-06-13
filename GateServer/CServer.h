#pragma once

#include "const.h"
#include "Httpconnection.h"

class CServer:public std::enable_shared_from_this<CServer>{
private:
	tcp::acceptor _acceptor;
	net::io_context& _ioc;
public:
	CServer(net::io_context& ioc, unsigned short& port);
	void start();
};

