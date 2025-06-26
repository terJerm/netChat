#pragma once

#include "const.h"
#include "Singleton.h"

class HttpConnection;
typedef std::function<void(std::shared_ptr<HttpConnection>)>httpHandler; //回调函数类型

class LogicSystem:public Singleton<LogicSystem>{
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem();
	//注册一个 get 请求
	void RegGet(std::string, httpHandler handler);    // 路由地址、会话对象
	void RegPost(std::string, httpHandler handler);
	//处理一个 get 请求
	bool handleGet(std::string , std::shared_ptr<HttpConnection>);//路由地址、会话对象ptr
	bool handlePost(std::string, std::shared_ptr<HttpConnection>);
private:
	LogicSystem();
	//post请求和get请求的回调函数map，key为路由，value为回调函数。
	std::map<std::string, httpHandler> _post_handlers;
	std::map<std::string, httpHandler> _get_handlers;
};











