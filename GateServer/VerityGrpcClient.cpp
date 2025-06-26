#include "VerityGrpcClient.h"
#include "ConfigMgr.h"


VerityGrpcClient::VerityGrpcClient() {
	std::string host = ConfigMgr::getInstance()["VarifyServer"]["host"];
	std::string port = ConfigMgr::getInstance()["VarifyServer"]["port"];
	_pool.reset(new RPConPool(5, host, port));
	/*
	reset:释放旧对象，可选的接管新对象
	ptr.reset();           // 释放当前对象，ptr 变为 nullptr
	ptr.reset(new T());    // 释放旧对象，并接管新对象
	*/
}


GetVarifyRsp VerityGrpcClient::getVarityCode(std::string email) {
	ClientContext context;
	GetVarifyRsp reply;
	GetVarifyReq request;
	request.set_email(email);

	//同步 RPC（远程过程调用）方法，用于向服务端发送请求并同步等待响应
	auto con = _pool->getConnection();
	Status status = con->GetVarifyCode(&context, request, &reply);    //通过stub 代理远程访问服务端的GetVarifyCode

	if (status.ok()) {
		reply.set_error(ErrorCodes::SUCCESS);
		//返还给池子
		_pool->returnConnection(std::move(con));
		return reply;
	}
	else {
		reply.set_error(ErrorCodes::RPCFAILED);
		//返还给池子
		_pool->returnConnection(std::move(con));
		return reply;
	}
}




//grpc连接池

RPConPool::RPConPool(std::size_t poolsize, std::string host, std::string port):
	_pool_size(poolsize),_host(host),_port(port),_stop(false){
	for (std::size_t q = 0; q < _pool_size; q++) {
		//创建融信通道，非安全凭证
		std::shared_ptr<Channel> channel = grpc::CreateChannel(_host + ":" + _port, grpc::InsecureChannelCredentials());
		_connections.push(VarifyService::NewStub(channel));  //将stub放入队列
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


void RPConPool::returnConnection(std::unique_ptr<VarifyService::Stub>context) {
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

void RPConPool::close() {
	_stop = true;
	_cond.notify_all();
}
