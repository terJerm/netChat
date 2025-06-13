#pragma once
#include <mutex>
#include "SingletonPtr.h"
#include <memory>
#include <unordered_map>

class CSession;

class UserMgr:public SingletonPtr<UserMgr>{
	friend class SingletonPtr<UserMgr>;
private:
	UserMgr();
	std::unordered_map<int, std::shared_ptr<CSession>>_uid_to_session;;
	std::mutex _session_mtx;;
public:
	~UserMgr();
	std::shared_ptr<CSession> GetSession(int uid);
	void SetUserSession(int uid, std::shared_ptr<CSession> session);
	void RmvUserSession(int uid);
	void RmvUserSession(int uid, std::string session_id);
};



