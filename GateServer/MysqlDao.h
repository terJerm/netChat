#pragma once

#include "const.h"
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/exception.h>


class SqlConnection {
public:
	int64_t _lasta_oper_time;                 //��������ʱ��
	std::unique_ptr<sql::Connection> _con;    //���ӵ�����ָ��
	SqlConnection(sql::Connection* con, int64_t lasttime);
};

struct UserInfo {
	std::string name;
	std::string pwd;
	int uid;
	std::string email;
};


// mysql ���ӳ�

class MysqlPool {
private:
	std::string _url;             //���ݿ��ַ
	std::string _user;
	std::string _passwd;
	std::string _schema;          //���ݿ�����
	std::size_t _poolsize;
	std::queue<std::unique_ptr<SqlConnection>> _pool;
	std::mutex _mutex;
	std::condition_variable _cond;
	std::atomic<bool > _stop;
	std::thread _check_thread;    //����̣߳�ÿ��һ��ʱ���mysql ����һ���źţ���ֹ̫��û����ʹmysql �ϵ�����
public:
	MysqlPool(const std::string& url, const std::string& user, std::string& pass, const std::string& schema,
		int poolsize);
	void checkConnection();
	std::unique_ptr<SqlConnection> getConnection();
	void returnConnection(std::unique_ptr<SqlConnection> con);
	void Close();
	~MysqlPool();
};



// mysql ����������
class MysqlDao{
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

};

