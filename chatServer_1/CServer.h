#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <map>

class CSession;


class CServer:public std::enable_shared_from_this<CServer>{
private:
	boost::asio::io_context& _io;
	boost::asio::ip::tcp::acceptor _acceptor;
	unsigned short _port;
	std::mutex _mutex;
	std::map<std::string, std::shared_ptr<CSession>> _sessions;

public:
	CServer(boost::asio::io_context& io, unsigned short);
	~CServer();
	void ClearSession(std::string);
	void StartAccept();

	void HandleAccept(std::shared_ptr<CSession> new_session, const boost::system::error_code& error);
};

