#pragma once

#include <memory>
#include <thread>
#include <iostream>
#include "SingletonPtr.h"
#include <map>
#include <functional>
#include "MsgNode.h"
#include <queue>
#include <unordered_map>
#include <json/json.h>

class CSession;
class LogicNode;
class UserInfo;
class CServer;


//逻辑队列读取消息
class LogicSystem:public SingletonPtr<LogicSystem>{
	friend class SingletonPtr<LogicSystem>;
	using FunCallBack = std::function<void(std::shared_ptr<CSession>, const short& msg_id, const std::string& msg_data)>;
private:
	std::condition_variable _cond;
	std::mutex _mutex;
	std::thread _work_thread;
	std::map<short, FunCallBack> _fun_call_back;             //读操作处理回调map   ,   //msg_id, func
	std::queue<std::shared_ptr<LogicNode>> _msg_que;         //消息队列

	std::shared_ptr<CServer> _p_server;

	bool _stop;      

	LogicSystem();
public:
	~LogicSystem();
	void work();
	void RegisterCallBacks();
	void stop();
	void PostMsgToQue(std::shared_ptr<LogicNode>);

	bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);

	void SetServer(std::shared_ptr<CServer> pserver);

private:
	bool isPureDigit(const std::string& str);
	void GetUserByUid(const std::string& uid_str, Json::Value&rtvalue);
	void GetUserByName(const std::string& uid_str, Json::Value&rtvalue);
 };

