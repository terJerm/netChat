#include "StatusGrpcClient.h"
#include "Defer.h"


StatusConPool::StatusConPool(std::size_t poolsize, std::string host, std::string port) :
	_pool_size(poolsize), _host(host), _port(port), _stop(false) {
	for (std::size_t q = 0; q < _pool_size; q++) {
		//创建融信通道
		std::shared_ptr<Channel> channel = grpc::CreateChannel(_host + ":" + _port, grpc::InsecureChannelCredentials());
		_connections.push(StatusServer::NewStub(channel));  //将stub放入队列
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
	从池中获取一个stub连接。
	如果需要退出或者池中没有空闲的，则使用wait挂起等待，直到被唤醒再取；（false）
	否则取出或退出（true）
	*/
	std::unique_lock<std::mutex>lock(_mutex);
	_cond.wait(lock, [this] {
		if (_stop) return true;
		return !_connections.empty();
		});
	//需要停止或者不为空是走到下面来
	if (_stop) return nullptr;
	//取出
	auto context = std::move(_connections.front());
	_connections.pop();
	return context;
}


void StatusConPool::returnConnection(std::unique_ptr<StatusServer::Stub>context) {
	/*
	//返还一个stub连接
	将 context 使用移动构造到连接池中
	唤起条件变量挂起的线程（取连接线程）
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
//通过状态服务器获取聊天服务器的信息
GetChatServerRsp StatusGrpcClient::GetChatServer(int uid) {
	ClientContext context;
	GetChatServerRsp reply;
	GetChatServerReq request;
	request.set_uid(uid);
	auto stub = _pool->getConnection();

	//通过代理远程访问
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