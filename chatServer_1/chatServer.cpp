

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
		// REDIS ���÷������ĵ�¼������ʼ��Ϊ0
		RedisMgr::getInstance()->HSet(LOGIN_COUNT, server_name, "0");
		Defer derfer([server_name]() {
			RedisMgr::getInstance()->HDel(LOGIN_COUNT, server_name);
			RedisMgr::getInstance()->Close();
			});


		// ���� boost ����
		boost::asio::io_context  io_context;
		boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
		auto port_str = (*cfg)["selfServer"]["Port"];
		//CServer s(io_context, atoi(port_str.c_str()));
		auto pointer_server = std::make_shared<CServer>(io_context, atoi(port_str.c_str()));

		//���� GrpcServer ����
		std::string server_address((*cfg)["selfServer"]["Host"] + ":" + (*cfg)["selfServer"]["RPCPort"]);
		ChatServiceImpl service;
		grpc::ServerBuilder builder;
		// �����˿ں���ӷ���
		builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
		builder.RegisterService(&service);
		service.RegisterServer(pointer_server);           // �� cserver ע��� grpc �����
		// ����������gRPC������
		std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
		std::cout << "RPC Server listening on " << server_address << std::endl;

		//��������һ���̴߳���grpc����
		std::thread  grpc_server_thread([&server]() {
			server->Wait();
			});

		signals.async_wait([&io_context, pool, &server](auto, auto) {
			io_context.stop();
			pool->Stop();
			server->Shutdown();
			});

		LogicSystem::getInstance()->SetServer(pointer_server);   /// ע����߼�ϵͳ
		io_context.run();
		// REDIS -----------------------------------------------------------------�����ϣ���й��ڱ����������ֶ�
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








