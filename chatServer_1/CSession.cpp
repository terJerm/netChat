#include "CSession.h"
#include <iostream>
#include "CServer.h"
#include "LogicSystem.h"
#include "RedisMgr.h"
#include "ConfigMgr.h"

#define HeartBeatTime 40       //心跳超时时间间隔

bool CSession::IsHeartBeatExpired(std::time_t& now)
{
	double difftime = std::difftime(now, _last_time_heart);
	if (difftime > HeartBeatTime) {
		std::cout << "心跳超时，session is : " << _session_id << std::endl;
		return true;
	}
	return false;
}

void CSession::updateHeartBeat(){
	time_t now = time(nullptr);
	_last_time_heart = now;
}

CSession::CSession(boost::asio::io_context& io, CServer* server):
	_socket(io),_service(server),_close(false), _b_head_parse(false),_data("") {

	boost::uuids::uuid uuid = boost::uuids::random_generator()();
	_session_id = boost::uuids::to_string(uuid);

	//初始化好接收头部信息的节点，等待start()接收
	_recv_head_node = std::make_shared<MsgNode>(HEAD_TOTAL_LEN);
}


void CSession::Start() {
	std::cout << "CSession start run ..." << std::endl;
	updateHeartBeat();
	ASyncReadHead(HEAD_TOTAL_LEN);      //读取消息头部
}
tcp::socket& CSession::getSocket() {
	return _socket;
}
std::string& CSession::getUuid() {
	return _session_id;
}
void CSession::Close() {
	std::lock_guard<std::mutex> lock(_session_mtex);
	_socket.close();
	_close = true;
}

//读取消息头部
void CSession::ASyncReadHead(int len) { 
	auto self = shared_from_this();
	//确保读满头部 4 字节，再触发回调函数
	asyncReadFull(HEAD_TOTAL_LEN, [this, self](const boost::system::error_code& ec, std::size_t bytes_transfered) {

		std::cout << "callBack run ..." << std::endl;
		try {
			if (ec) {                 //读取头部4字节的回调出现错误
				std::cout << "handle read failed, error is " << ec.what() << std::endl;
				Close();

				DealExceptionSession();// 客户端关闭连接了，加锁清除session
				_service->ClearSession(_session_id);
				return;
			}
			if (bytes_transfered < HEAD_TOTAL_LEN) { //并没有读取到头部四字节
				std::cout << "read length not match, read [" << bytes_transfered << "] , total ["<< HEAD_TOTAL_LEN << "]" << std::endl;
				Close();
				_service->ClearSession(_session_id);
				return;
			}

			if (!_service->CheckSessionValid(_session_id)) {
				Close();
				return;
			}
			// 更新心跳
			updateHeartBeat();

			_recv_head_node->clear();
			memcpy(_recv_head_node->_data, _data, bytes_transfered);  //将_data中临时头字节数据放到头节点中
			//读取消息头中的id
			short msg_id = 0;
			memcpy(&msg_id, _recv_head_node->_data, HEAD_ID_LEN);
			msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);

			//读取消息头中的len
			short msg_len = 0;
			memcpy(&msg_len, _recv_head_node->_data + HEAD_ID_LEN, HEAD_DATA_LEN);
			msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);
			if (msg_len > MAX_LENGTH) {
				std::cout << "invalid data length is " << msg_len << std::endl;
				_service->ClearSession(_session_id);
				return;
			}
			_recv_msg_node = std::make_shared<RecvNode>(msg_len, msg_id);

			//读取消息体
			AsyncReadBody(msg_len);
		}
		catch (std::exception& e) {
			std::cout << "Exception code is " << e.what() << std::endl;
		}
		});
}

//确保读满n字节，再触发回调函数
void CSession::asyncReadFull(std::size_t maxLength, std::function<void(const boost::system::error_code&, std::size_t)>handler) {
	::memset(_data, 0, MAX_LENGTH);
	asyncReadLen(0, maxLength, handler);
}

//从read_len(读取的字节数)开始，读满total_len个字节后，触发回调函数handler
void CSession::asyncReadLen(std::size_t read_len, std::size_t total_len,
	std::function<void(const boost::system::error_code&, std::size_t)> handler) {

	auto self = shared_from_this();
	/*
	通过 boost::asio::buffer(_data + read_len, total_len - read_len) 限定了读入的位置和长度
	通过buffer只开放4字节空间大小，让其最多只能读满消息头，保证消息体的完整性
	*/
	std::cout << "begin async read some  ..." << std::endl;

	try {
	_socket.async_read_some(boost::asio::buffer(_data + read_len, total_len - read_len),
		[read_len, total_len, self, handler](const boost::system::error_code& er, std::size_t  bytesTransfered) {
			if (er) {                                       //出现错误
				handler(er, read_len + bytesTransfered);
				return;
			}
			if (read_len + bytesTransfered >= total_len) {  //头部 4 字节读取完了，触发回调
				handler(er, read_len + bytesTransfered);
				return;
			}
			// 没有错误，且长度不足则继续读取
			self->asyncReadLen(read_len + bytesTransfered, total_len, handler);
		});
	}
	catch (std::exception& e) {
		std::cout << "asyncRead failed , Exception code is " << e.what() << std::endl;
		std::cout << "正在关闭此会话..." << std::endl;

		_service->ClearSession(_session_id);
		return;
	}


}

//读取消息体
void CSession::AsyncReadBody(int total_len) {
	auto self = shared_from_this();
	asyncReadFull(total_len, [this,self,total_len](const boost::system::error_code& er, std::size_t bytes_transfered) {
		try {
			if (er) {
				std::cout << "handle read failed, error is " << er.what() << std::endl;
				Close();

				DealExceptionSession();// 客户端关闭连接了，加锁清除session

				_service->ClearSession(_session_id);
				return;
			}
			if (bytes_transfered < total_len) {
				std::cout << "read length not match, read [" << bytes_transfered << "] , total ["<< total_len << "]" << std::endl;
				Close();
				_service->ClearSession(_session_id);
				return;
			}

			if (!_service->CheckSessionValid(_session_id)) {
				Close();
				return;
			}
			// 更新心跳
			updateHeartBeat();

			memcpy(_recv_msg_node->_data, _data, bytes_transfered);
			_recv_msg_node->_cur_len += bytes_transfered;
			_recv_msg_node->_data[_recv_msg_node->_total_len] = '\0';
			std::cout << "receive data is " << _recv_msg_node->_data << std::endl;

			//此处将消息投递到逻辑单例的逻辑队列中，逻辑单例中会有一个线程从队列中不断取出数据进行处理
			LogicSystem::getInstance()->PostMsgToQue(std::make_shared<LogicNode>(shared_from_this(), _recv_msg_node));
			//继续监听头部接受事件
			ASyncReadHead(HEAD_TOTAL_LEN);

		}
		catch (std::exception& e) {
			std::cout << "Exception code is " << e.what() << std::endl;
		}
		});
}


void CSession::Send(char* msg, short max_len, short msgid) {
	std::lock_guard<std::mutex>lock(_mutex);
	auto self = shared_from_this();
	int send_que_size = _send_que.size();
	if (send_que_size >= MAX_SENDQUE) {
		std::cout << "session: " << _session_id << " send que fulled, size is " << MAX_SENDQUE << std::endl;
		return;
	}
	_send_que.push(std::make_shared<SendNode>(msg, max_len, msgid));
	if (send_que_size > 0) {                  //队列中存在数据，排队
		return;
	}
	do_Send();
}

//发送消息，消息队列中的第一个消息
void CSession::do_Send() {
	auto self = shared_from_this();
	auto& msgnode = _send_que.front();

	boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
		[self, this](const boost::system::error_code& er, std::size_t bytes_transferred) {
			try {
				if (er) {
					std::cout << "handle write failed, error is " << er.what() << std::endl;
					Close();

					DealExceptionSession();// 客户端关闭连接了，加锁清除session
					_service->ClearSession(_session_id);
					return;
				}
				std::lock_guard<std::mutex>lock(_mutex);
				this->_send_que.pop();
				if (!this->_send_que.empty()) {
					do_Send();
				}
			}
			catch (std::exception& e) {
				std::cerr << "Exception code : " << e.what() << std::endl;
			}
		});
	
}

void CSession::Send(std::string msg, short msgid) {
	std::lock_guard<std::mutex> lock(_mutex);
	int send_que_size = _send_que.size();
	if (send_que_size > MAX_SENDQUE) {
		std::cout << "session: " << _session_id << " send que fulled, size is " << MAX_SENDQUE << std::endl;
		return;
	}
	_send_que.push(std::make_shared<SendNode>(msg.c_str(), static_cast<short>(msg.length()), msgid));
	if (send_que_size > 0) {
		std::cout << "wait to send ..." << std::endl;
		return;
	}
	std::cout << "sending ..." << std::endl;
	do_Send();
	
}
CSession::~CSession() {
	std::cout << "客户端关闭，此 session 析构调用..." << std::endl;

	//减少服务器登录数量
	auto cfg = ConfigMgr::getInstance();
	auto self_name = (*cfg)["selfServer"]["Name"];
	RedisMgr::getInstance()->DecreaseCount(self_name);
}

void CSession::SetUserId(int uid) {
	_user_id = uid;
}
int CSession::GetUserId() {
	return _user_id;
}

/// <summary>
///  清理断开连接的客户端
/// </summary>
void CSession::DealExceptionSession(){
	auto self = shared_from_this();
	// 加锁清除 session
	auto uid_str = std::to_string(_user_id);
	auto lock_key = LOCK_PREFIX + uid_str;
	auto identifier = RedisMgr::getInstance()->acquireLock(lock_key, LOCK_TIME_OUT, ACQUIRE_TIME_OUT);
	Defer defer([identifier, lock_key, self, this]() {
		_service->ClearSession(_session_id);
		RedisMgr::getInstance()->releaseLock(lock_key, identifier);
		});

	if (identifier.empty()) {
		return;
	}
	std::string redis_session_id = "";
	auto bsuccess = RedisMgr::getInstance()->Get(USER_SESSION_PREFIX + uid_str, redis_session_id);
	if (!bsuccess) {
		return;
	}

	if (redis_session_id != _session_id) {
		// 说明有其他客户端异地登陆了，以为是加了分布式锁的，所以是对方登录之后这边才退出的
		return;
	}

	RedisMgr::getInstance()->Del(USER_SESSION_PREFIX + uid_str);
	
	RedisMgr::getInstance()->Del(USERIPPREFIX + uid_str);        // 清除用户登录信息,使之离线状态
}

void CSession::NotifyOffline(int uid)
{
	Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["uid"] = uid;

	std::string return_str = rtvalue.toStyledString();

	Send(return_str, ID_NOTIFY_OFF_LINE_REQ);
	return;
}

std::string& CSession::GetSessionId()
{
	return _session_id;
}


 







LogicNode::LogicNode(std::shared_ptr<CSession> session, std::shared_ptr<RecvNode> node)
	:_session(session), _recvnode(node) {
}



