

#include "const_.h"
#include <thread>
#include <mutex>
#include <csignal>
#include <iostream>
#include "ConfigMgr.h"
#include "AsioIOServicePool.h"
#include "CServer.h"
#include "RedisMgr.h"
#include "ChatServiceImpl.h"
#include "LogicSystem.h"

//[Redis]
//Host = 81.68.86.146
//Port = 6380
//Passwd = 123456

std::error_condition cond_quit;
std::mutex mutex_quit;

int main() {
	auto cfg = ConfigMgr::getInstance();
	auto server_name = (*cfg)["selfServer"]["Name"];
	try {
		auto pool = AsioIOServicePool::getInstance();
		// REDIS 将该服务器的登录数量初始化为0
		RedisMgr::getInstance()->HSet(LOGIN_COUNT, server_name, "0");
		Defer derfer([server_name]() {
			RedisMgr::getInstance()->HDel(LOGIN_COUNT, server_name);
			RedisMgr::getInstance()->Close();
			});


		// 启动 boost 服务
		boost::asio::io_context  io_context;
		boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
		auto port_str = (*cfg)["selfServer"]["Port"];
		//CServer s(io_context, atoi(port_str.c_str()));
		auto pointer_server = std::make_shared<CServer>(io_context, atoi(port_str.c_str()));

		//启动 GrpcServer 服务
		std::string server_address((*cfg)["selfServer"]["Host"] + ":" + (*cfg)["selfServer"]["RPCPort"]);
		ChatServiceImpl service;
		grpc::ServerBuilder builder;
		// 监听端口和添加服务
		builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
		builder.RegisterService(&service);
		service.RegisterServer(pointer_server);           // 把 cserver 注册进 grpc 服务端
		// 构建并启动gRPC服务器
		std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
		std::cout << "RPC Server listening on " << server_address << std::endl;

		//单独启动一个线程处理grpc服务
		std::thread  grpc_server_thread([&server]() {
			server->Wait();
			});

		signals.async_wait([&io_context, pool, &server](auto, auto) {
			io_context.stop();
			pool->Stop();
			server->Shutdown();
			});

		LogicSystem::getInstance()->SetServer(pointer_server);   /// 注册给逻辑系统
		io_context.run();
		// REDIS -----------------------------------------------------------------清除哈希表中关于本服务器的字段
		RedisMgr::getInstance()->HDel(LOGIN_COUNT, server_name);
		RedisMgr::getInstance()->Close();
		grpc_server_thread.join();
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		RedisMgr::getInstance()->HDel(LOGIN_COUNT, server_name);
		RedisMgr::getInstance()->Close();
	}
}








