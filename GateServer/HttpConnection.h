#pragma once

#include "const.h"

//连接会话类
class HttpConnection:public std::enable_shared_from_this<HttpConnection>{
	friend class LogicSystem;
private:
	tcp::socket _socket;                                //连接的对象
	beast::flat_buffer _buffer{ 8192 };                 //用来接收数据
	http::request<http::dynamic_body> _request;         //请求对象，用来解析请求
	http::response<http::dynamic_body> _response;       //响应对象，用来回应
	net::steady_timer _deadline{                        //定时器判断是否超时
		_socket.get_executor(),std::chrono::seconds(10)
	};

	std::string _get_url;                                             //解析出的整个 url
	std::unordered_map<std::string, std::string> _get_params;         //解析的参数：key:value

public:
	HttpConnection(boost::asio::io_context& ioc);
	void start();
	tcp::socket& getSocket();
private:
	void CheckDeadline();  //超时检测
	void WriteResponce();  //应答回复
	void HandleReq();      //处理请求
	void PreParseGetParam();               ////get 请求的参数解析

};

