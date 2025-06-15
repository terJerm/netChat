#pragma once


#include "MysqlDao.h"
#include "SingletonPtr.h"
#include <json/value.h>

class UserInfo;


//接口層，頂層封裝，提供外部調用的接口
class MysqlMgr :public SingletonPtr<MysqlMgr> {
	friend class SingletonPtr<MysqlMgr>;
private:
	MysqlMgr();
	MysqlDao _dao;
public:
	~MysqlMgr();
	int regUser(const std::string& name, const std::string& email, const std::string& pwd);
	bool CheckEmail(const std::string& name, const std::string& email);
	bool UpdatePwd(const std::string& name, const std::string& email);
	bool CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo);
	bool addFriendApply(const int&, const int&, const std::string&, const std::string&);
	bool AuthFriendApply(int, int);
	bool AddFriend(int, int);
	std::shared_ptr<UserInfo> GetUser(int uid);
	std::shared_ptr<UserInfo> GetUser(std::string uid);
	bool addMessageNode(const int& senderuid, const int& receiveruid, std::string& message, std::string& marking, bool& b_ip,int type);

	bool getFriendListFromUid(int& uid, Json::Value& rtvalue);
	bool getFriendRequestListFromUid(int& uid, Json::Value& rtvalue);
	bool getMessageListFromUid(int& selfuid, int& useruid, Json::Value& rtvalue);
	bool getMoreMessageListFromUid(int& selfuid, int& useruid, std::string& marking ,Json::Value& rtvalue);
};
