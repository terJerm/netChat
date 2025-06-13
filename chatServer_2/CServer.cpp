#include "CServer.h"
#include "AsioIOServicePool.h"
#include "CSession.h"
#include "UserMgr.h"

CServer::CServer(boost::asio::io_context& io, unsigned short port) try
	: _io(io),
	_port(port),
	_acceptor(io, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
	/*
	C++ 类的成员初始化顺序 不是你写的顺序，而是按照声明顺序！!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!坑死我了
	*/
{
	StartAccept();
}
catch (const std::exception& e) {
	std::cerr << "CServer Init Error: " << e.what() << std::endl;
	throw; // 确保异常传递到主函数
}

CServer::~CServer() {
	std::cout << "Server destruct listen on port : " << _port << std::endl;
}

void CServer::StartAccept() {
	auto& io_context = AsioIOServicePool::getInstance()->getIOContext();
	std::shared_ptr<CSession> new_session = std::make_shared<CSession>(io_context, this);
	_acceptor.async_accept(new_session->getSocket(), std::bind(&CServer::HandleAccept, this, new_session, std::placeholders::_1));
}

void CServer::HandleAccept(std::shared_ptr<CSession> new_session, const boost::system::error_code& error) {
	if (!error) {
		new_session->Start();
		std::lock_guard<std::mutex> lock(_mutex);
		_sessions.insert(make_pair(new_session->getUuid(), new_session));
	}
	else {
		std::cout << "session accept failed, error is " << error.what() << std::endl;
	}
	StartAccept();
}



void CServer::ClearSession(std::string uuid) {

	std::lock_guard<std::mutex> lock(_mutex);
	if (_sessions.find(uuid) != _sessions.end()) {
		// 移出 用户和 ssession  的关联
		UserMgr::getInstance()->RmvUserSession(_sessions[uuid]->GetUserId());
	}

	_sessions.erase(uuid);
}
