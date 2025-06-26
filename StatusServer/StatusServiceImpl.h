#pragma once
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include <mutex>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::LoginReq;
using message::LoginRsp;
using message::StatusServer;

struct ChatServer {
	std::string host;
	std::string port;
	std::string name;
	int con_count = 0;
};
class StatusServiceImpl final : public StatusServer::Service
{
public:
	StatusServiceImpl();
	Status GetChatServer(ServerContext* context, const GetChatServerReq* request,
		GetChatServerRsp* reply) override;
	Status Login(ServerContext* context, const LoginReq* request,
		LoginRsp* reply) override;
private:
	void insertToken(int uid, std::string token);
	ChatServer getChatServer();                                     //轮询找出压力最小的聊天服务器
	std::unordered_map<std::string, ChatServer> _servers;           //聊天服务的名字，该服务器的信息
	std::mutex _server_mtx;
};