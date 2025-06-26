#include "StatusGrpcClient.h"
#include "Defer.h"


StatusConPool::StatusConPool(std::size_t poolsize, std::string host, std::string port) :
	_pool_size(poolsize), _host(host), _port(port), _stop(false) {
	for (std::size_t q = 0; q < _pool_size; q++) {
		//��������ͨ��
		std::shared_ptr<Channel> channel = grpc::CreateChannel(_host + ":" + _port, grpc::InsecureChannelCredentials());
		_connections.push(StatusServer::NewStub(channel));  //��stub�������
	}
}

StatusConPool::~StatusConPool() {
	std::lock_guard<std::mutex> lock(_mutex);
	close();
	while (!_connections.empty()) {
		_connections.pop();
	}
}

std::unique_ptr<StatusServer::Stub> StatusConPool::getConnection() {
	/*
	�ӳ��л�ȡһ��stub���ӡ�
	�����Ҫ�˳����߳���û�п��еģ���ʹ��wait����ȴ���ֱ����������ȡ����false��
	����ȡ�����˳���true��
	*/
	std::unique_lock<std::mutex>lock(_mutex);
	_cond.wait(lock, [this] {
		if (_stop) return true;
		return !_connections.empty();
		});
	//��Ҫֹͣ���߲�Ϊ�����ߵ�������
	if (_stop) return nullptr;
	//ȡ��
	auto context = std::move(_connections.front());
	_connections.pop();
	return context;
}


void StatusConPool::returnConnection(std::unique_ptr<StatusServer::Stub>context) {
	/*
	//����һ��stub����
	�� context ʹ���ƶ����쵽���ӳ���
	������������������̣߳�ȡ�����̣߳�
	*/
	std::lock_guard<std::mutex>lock(_mutex);
	if (_stop) return;
	_connections.push(std::move(context));
	_cond.notify_one();
}

void StatusConPool::close() {
	_stop = true;
	_cond.notify_all();
}






StatusGrpcClient::StatusGrpcClient() {
	auto& mgr = ConfigMgr::getInstance();
	std::string host = mgr["StatusServer"]["Host"];
	std::string port = mgr["StatusServer"]["Port"];
	_pool.reset(new StatusConPool(5, host, port));
}
StatusGrpcClient::~StatusGrpcClient() {

}
LoginRsp StatusGrpcClient::Login(int uid, std::string token) {
	ClientContext context;
	LoginRsp reply;
	LoginReq request;
	request.set_uid(uid);
	request.set_token(token);

	auto stub = _pool->getConnection();
	Status status = stub->Login(&context, request, &reply);
	Defer defer([&stub, this]() {
		_pool->returnConnection(std::move(stub));
		});
	if (status.ok()) {
		return reply;
	}
	else {
		reply.set_error(ErrorCodes::RPCFailed);
		return reply;
	}
}
//ͨ��״̬��������ȡ�������������Ϣ
GetChatServerRsp StatusGrpcClient::GetChatServer(int uid) {
	ClientContext context;
	GetChatServerRsp reply;
	GetChatServerReq request;
	request.set_uid(uid);
	auto stub = _pool->getConnection();

	//ͨ������Զ�̷���
	Status status = stub->GetChatServer(&context, request, &reply);
	Defer defer([&stub, this]() {
		_pool->returnConnection(std::move(stub));
		});
	if (status.ok()) return reply;
	else {
		reply.set_error(ErrorCodes::RPCFailed);
		return reply;
	}
}