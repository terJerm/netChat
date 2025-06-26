#pragma once

#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "const.h"
#include "Singleton.h"

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetVarifyReq;
using message::GetVarifyRsp;
using message::VarifyService;

// gRPC 客户端封装类，用于与 gRPC 服务端进行通信，主要功能是发送验证码请求并获取响应。

class RPConPool;

class VerityGrpcClient:public Singleton<VerityGrpcClient>{
	friend class Singleton<VerityGrpcClient>;
private:
	std::unique_ptr<RPConPool> _pool;       //stub连接池

	VerityGrpcClient();

public:
	GetVarifyRsp getVarityCode(std::string email);
};


//封装一个grpc连接池
class RPConPool {
private:
	std::atomic<bool> _stop;
	std::size_t _pool_size;
	std::string _host;
	std::string _port;
	std::mutex _mutex;
	std::condition_variable _cond;
	std::queue < std::unique_ptr<VarifyService::Stub>> _connections; //Stub（存根）是客户端调用远程服务的本地代理对象

public:
	RPConPool(std::size_t poolsize, std::string host, std::string port);
	~RPConPool();
	//从池中获取一个stub 连接
	std::unique_ptr<VarifyService::Stub> getConnection();
	//返还一个stub连接
	void returnConnection(std::unique_ptr<VarifyService::Stub>context);
	void close();
};