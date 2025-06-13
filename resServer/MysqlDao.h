#pragma once

#include "const.h"
#include <thread>
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/exception.h>
#include <memory>
#include <queue>
#include <mutex>
#include <json/value.h>

class SqlConnection {
public:
	int64_t _lasta_oper_time;                 //最后操作的时间
	std::unique_ptr<sql::Connection> _con;    //连接的智能指针
	SqlConnection(sql::Connection* con, int64_t lasttime);
};

//---------------------------------------------------------------------------------------------------------

class MysqlPool {
private:
	std::string _url;             //数据库地址
	std::string _user;
	std::string _passwd;
	std::string _schema;          //数据库名字
	std::size_t _poolsize;
	std::queue<std::unique_ptr<SqlConnection>> _pool;
	std::mutex _mutex;
	std::condition_variable _cond;
	std::atomic<bool > _stop;
	std::thread _check_thread;    //检测线程，每隔一段时间给mysql 发送一个信号，防止太久没操作使mysql 断掉连接
public:
	MysqlPool(const std::string& url, const std::string& user, std::string& pass, const std::string& schema,
		int poolsize);
	void checkConnection();
	std::unique_ptr<SqlConnection> getConnection();
	void returnConnection(std::unique_ptr<SqlConnection> con);
	void Close();
	~MysqlPool();
};



// mysql 操作管理类
class MysqlDao {
	using string = std::string;
private:
	std::unique_ptr<MysqlPool> _sqlpool;
public:
	MysqlDao();
	~MysqlDao();
	int regUser(const string& name, const string& email, const string& pwd);
	bool CheckEmail(const std::string& name, const std::string& email);
	bool UpdatePwd(const std::string& name, const std::string& pwd);
	bool CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo);
	bool addFriendApply(const int&, const int&, const std::string&, const std::string&);
	bool AuthFriendApply(int selfuid, int touid);
	bool AddFriend(int selfuid, int touid);
	std::shared_ptr<UserInfo> GetUser(int uid);
	std::shared_ptr<UserInfo> GetUser(std::string uid);
	bool addMessageNode(const int& senderuid, const int& receiveruid, std::string& message, std::string& marking, bool& b_ip,int type);

	bool getFriendListFromUid(int& uid, Json::Value& rtvalue);
	bool getFriendRequestListFromUid(int& uid, Json::Value& rtvalue);
	bool getMessageListFromUid(int& selfuid, int& useruid, Json::Value& rtvalue);
	bool getMoreMessageListFromUid(int& selfuid, int& touid, std::string& marking, Json::Value& rtvalue);

	bool insertFileInfo(std::string& filemarking, std::string& filename, int64_t& filesize, int& uid, std::string& filepath, std::string& tip);

	bool getFilePathFromMarking(const std::string& marking, fileType& filepath);

};



