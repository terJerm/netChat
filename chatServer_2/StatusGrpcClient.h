#pragma once

#include "SingletonPtr.h"
#include "const_.h"
#include "ConfigMgr.h"
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <queue>



using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::LoginRsp;
using message::LoginReq;
using message::StatusService;

	
//封装一个grpc连接池
class StatusConPool {
private:
	std::atomic<bool> _stop;
	std::size_t _pool_size;
	std::string _host;
	std::string _port;
	std::mutex _mutex;
	std::condition_variable _cond;
	std::queue< std::unique_ptr<StatusService::Stub>> _connections; //Stub（存根）是客户端调用远程服务的本地代理对象

public:
	StatusConPool(std::size_t poolsize, std::string host, std::string port);
	~StatusConPool();
	//从池中获取一个stub 连接
	std::unique_ptr<StatusService::Stub> getConnection();
	//返还一个stub连接
	void returnConnection(std::unique_ptr<StatusService::Stub>context);
	void close();
};

/*
状态服务器客户端
gateServer 网关服务器在用户登录时通过grpc 调用状态服务器查询合适的 chatServer ，
将其信息发送给客户端连接
*/
class StatusGrpcClient:public SingletonPtr<StatusGrpcClient>{
	friend class SingletonPtr<StatusGrpcClient>;
private:
	std::unique_ptr<StatusConPool> _pool;
	StatusGrpcClient();
public:
	~StatusGrpcClient();
	LoginRsp Login(int uid, std::string token);
	//通过状态服务器获取聊天服务器的信息
	GetChatServerRsp GetChatServer(int uid);
};

