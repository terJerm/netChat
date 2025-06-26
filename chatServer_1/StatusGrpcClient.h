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

	
//��װһ��grpc���ӳ�
class StatusConPool {
private:
	std::atomic<bool> _stop;
	std::size_t _pool_size;
	std::string _host;
	std::string _port;
	std::mutex _mutex;
	std::condition_variable _cond;
	std::queue< std::unique_ptr<StatusService::Stub>> _connections; //Stub��������ǿͻ��˵���Զ�̷���ı��ش������

public:
	StatusConPool(std::size_t poolsize, std::string host, std::string port);
	~StatusConPool();
	//�ӳ��л�ȡһ��stub ����
	std::unique_ptr<StatusService::Stub> getConnection();
	//����һ��stub����
	void returnConnection(std::unique_ptr<StatusService::Stub>context);
	void close();
};

/*
״̬�������ͻ���
gateServer ���ط��������û���¼ʱͨ��grpc ����״̬��������ѯ���ʵ� chatServer ��
������Ϣ���͸��ͻ�������
*/
class StatusGrpcClient:public SingletonPtr<StatusGrpcClient>{
	friend class SingletonPtr<StatusGrpcClient>;
private:
	std::unique_ptr<StatusConPool> _pool;
	StatusGrpcClient();
public:
	~StatusGrpcClient();
	LoginRsp Login(int uid, std::string token);
	//ͨ��״̬��������ȡ�������������Ϣ
	GetChatServerRsp GetChatServer(int uid);
};

