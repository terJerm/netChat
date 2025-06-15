#include <iostream>
#include "CServer.h"
#include "ConfigMgr.h"


int main(){
	auto& con = ConfigMgr::getInstance();
	std::string gate_port_str = con["GateServer"] ["Port"] ;
	unsigned short gate_port = atoi(gate_port_str.c_str());

	try {
		unsigned short port = static_cast<unsigned short>(gate_port);
		net::io_context ioc{ 1 };
		boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
		signals.async_wait([&ioc](const boost::system::error_code& err, int signal_number) {
			if (err) return;
			ioc.stop();
			});
		std::make_shared<CServer>(ioc, port)->start();
		std::cout << "Gate Server listen on port:"<<port << std::endl;
		ioc.run();
	}
	catch (std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
    }
}

/*
	grpc 已经编译好了，接下来就是在vs中配置 grpc 了：18：00

	那个属性表文件，在该项目目录下，需要导入到项目中来，并且替换成自己的目录。
	（运行会报错，因为 mysql 和 redus 还没有配置好）
*/


