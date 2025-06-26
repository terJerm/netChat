#pragma once

#include "MysqlDao.h"
#include "Singleton.h"

struct UserInfo;


//�ӿڌӣ�플ӷ��b���ṩ�ⲿ�{�õĽӿ�
class MysqlMgr:public SingletonPtr<MysqlMgr>{
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
};

