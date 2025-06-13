#include "VerityGrpcClient.h"
#include "ConfigMgr.h"


VerityGrpcClient::VerityGrpcClient() {
	std::string host = ConfigMgr::getInstance()["VarifyServer"]["host"];
	std::string port = ConfigMgr::getInstance()["VarifyServer"]["port"];
	_pool.reset(new RPConPool(5, host, port));
	/*
	reset:�ͷžɶ��󣬿�ѡ�Ľӹ��¶���
	ptr.reset();           // �ͷŵ�ǰ����ptr ��Ϊ nullptr
	ptr.reset(new T());    // �ͷžɶ��󣬲��ӹ��¶���
	*/
}


GetVarifyRsp VerityGrpcClient::getVarityCode(std::string email) {
	ClientContext context;
	GetVarifyRsp reply;
	GetVarifyReq request;
	request.set_email(email);

	//ͬ�� RPC��Զ�̹��̵��ã����������������˷�������ͬ���ȴ���Ӧ
	auto con = _pool->getConnection();
	Status status = con->GetVarifyCode(&context, request, &reply);    //ͨ��stub ����Զ�̷��ʷ���˵�GetVarifyCode

	if (status.ok()) {
		reply.set_error(ErrorCodes::SUCCESS);
		//����������
		_pool->returnConnection(std::move(con));
		return reply;
	}
	else {
		reply.set_error(ErrorCodes::RPCFAILED);
		//����������
		_pool->returnConnection(std::move(con));
		return reply;
	}
}




//grpc���ӳ�

RPConPool::RPConPool(std::size_t poolsize, std::string host, std::string port):
	_pool_size(poolsize),_host(host),_port(port),_stop(false){
	for (std::size_t q = 0; q < _pool_size; q++) {
		//��������ͨ�����ǰ�ȫƾ֤
		std::shared_ptr<Channel> channel = grpc::CreateChannel(_host + ":" + _port, grpc::InsecureChannelCredentials());
		_connections.push(VarifyService::NewStub(channel));  //��stub�������
	}
}

RPConPool::~RPConPool() {
	std::lock_guard<std::mutex> lock(_mutex);
	close();
	while (!_connections.empty()) {
		_connections.pop();
	}
}

std::unique_ptr<VarifyService::Stub> RPConPool::getConnection() {
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


void RPConPool::returnConnection(std::unique_ptr<VarifyService::Stub>context) {
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

void RPConPool::close() {
	_stop = true;
	_cond.notify_all();
}
