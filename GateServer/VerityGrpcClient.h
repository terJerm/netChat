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

// gRPC �ͻ��˷�װ�࣬������ gRPC ����˽���ͨ�ţ���Ҫ�����Ƿ�����֤�����󲢻�ȡ��Ӧ��

class RPConPool;

class VerityGrpcClient:public Singleton<VerityGrpcClient>{
	friend class Singleton<VerityGrpcClient>;
private:
	std::unique_ptr<RPConPool> _pool;       //stub���ӳ�

	VerityGrpcClient();

public:
	GetVarifyRsp getVarityCode(std::string email);
};


//��װһ��grpc���ӳ�
class RPConPool {
private:
	std::atomic<bool> _stop;
	std::size_t _pool_size;
	std::string _host;
	std::string _port;
	std::mutex _mutex;
	std::condition_variable _cond;
	std::queue < std::unique_ptr<VarifyService::Stub>> _connections; //Stub��������ǿͻ��˵���Զ�̷���ı��ش������

public:
	RPConPool(std::size_t poolsize, std::string host, std::string port);
	~RPConPool();
	//�ӳ��л�ȡһ��stub ����
	std::unique_ptr<VarifyService::Stub> getConnection();
	//����һ��stub����
	void returnConnection(std::unique_ptr<VarifyService::Stub>context);
	void close();
};