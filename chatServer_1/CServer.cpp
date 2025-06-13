#include "CServer.h"
#include "AsioIOServicePool.h"
#include "CSession.h"
#include "UserMgr.h"

CServer::CServer(boost::asio::io_context& io, unsigned short port) try
	: _io(io),
	_port(port),
	_acceptor(io, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
	/*
	C++ ��ĳ�Ա��ʼ��˳�� ������д��˳�򣬶��ǰ�������˳��!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!��������
	*/
{
	StartAccept();
}
catch (const std::exception& e) {
	std::cerr << "CServer Init Error: " << e.what() << std::endl;
	throw; // ȷ���쳣���ݵ�������
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
		// �Ƴ� �û��� ssession  �Ĺ���
		UserMgr::getInstance()->RmvUserSession(_sessions[uuid]->GetUserId());
	}

	_sessions.erase(uuid);
}
