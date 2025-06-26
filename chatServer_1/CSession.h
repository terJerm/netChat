#pragma once
#include <memory>
#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <queue>
#include <mutex>
#include "const_.h"
#include <boost/uuid/uuid_io.hpp>
#include "MsgNode.h"
#include <functional>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class CServer;
class LogicSystem;
class LogicNode;


class CSession:public std::enable_shared_from_this<CSession>{
private:
	tcp::socket _socket;
	std::string _session_id;                    //连接本次会话的用户的uuid
	int _user_id;
	CServer* _service;
	char _data[MAX_LENGTH];                          //存储临时读到的数据
	bool _close;

	std::queue<std::shared_ptr<SendNode>> _send_que;      //发送队列，由于是异步发送所以要使用队列保证有序性
	std::mutex _mutex;

	std::shared_ptr<RecvNode> _recv_msg_node;             //消息体，接收消息
	std::shared_ptr<MsgNode> _recv_head_node;	          //消息头
	bool _b_head_parse;

	std::atomic<time_t> _last_time_heart;                 //心跳时间戳
	std::mutex _session_mtex;
public:
	bool IsHeartBeatExpired(std::time_t& now);
	void updateHeartBeat();

	CSession(boost::asio::io_context& io, CServer* server);
	~CSession();
	void Start();
	tcp::socket& getSocket();
	std::string& getUuid();
	void Close();

	void ASyncReadHead(int len);
	void asyncReadFull(std::size_t maxLength, std::function<void(const boost::system::error_code&, std::size_t)>);
	void asyncReadLen(std::size_t read_len, std::size_t total_len,
		std::function<void(const boost::system::error_code&, std::size_t)> handler);
	void AsyncReadBody(int total_len);
	void Send(char* msg, short max_len,short msgid);
	void do_Send();
	void Send(std::string msg, short msgid);
	void SetUserId(int uid);
	int GetUserId();

	void DealExceptionSession();         /// 客户端退出了，清理客户端

	void NotifyOffline(int uid);
	std::string& GetSessionId();
};



class LogicNode {
	friend class LogicSystem;
private:
	std::shared_ptr<CSession> _session;
	std::shared_ptr<RecvNode> _recvnode;
public:
	LogicNode(std::shared_ptr<CSession>, std::shared_ptr<RecvNode>);
};