#include "MysqlDao.h"
#include "Defer.h"
#include "ConfigMgr.h"

SqlConnection::SqlConnection(sql::Connection* con, int64_t lasttime):
	_con(con), _lasta_oper_time(lasttime){

}

MysqlPool::MysqlPool(const std::string& url, const std::string& user, std::string& pass, const std::string& schema,
	int poolsize) :_url(url), _user(user), _passwd(pass), _schema(schema), _poolsize(poolsize), _stop(false) {
	try {
		std::cout << "_url is: " << _url << std::endl;
		std::cout << "_user is: " << _user << std::endl;
		std::cout << "_passwd is: " << _passwd << std::endl;
		std::cout << "_schema is: " << _schema << std::endl;


		for (int q = 0; q < _poolsize; q++) {
			sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
			//连接到 mysql 的数据库   
			sql::Connection* con = driver->connect(_url, _user, _passwd);
			con->setSchema(_schema);

			auto currentTime = std::chrono::system_clock::now().time_since_epoch();
			long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
			//压入连接池
			_pool.push(std::make_unique<SqlConnection>(con, timestamp));
			std::cout << "the "<<q<<" connection from mysqlpools connected  success!..." << std::endl;
		}
		_check_thread = std::thread([this]() {                          //心跳检测
			std::cout << "后台心跳线程启动 ..." << std::endl;
			while (!_stop) {
				checkConnection();
				std::this_thread::sleep_for(std::chrono::seconds(60));  //一分钟给mysql 发一次信号
			}
			});
		_check_thread.detach();  //挂到后台，让操作系统回收
	}
	catch (sql::SQLException& e) {
		std::cout << "MySQL pool init failed: " << e.what() << std::endl;
		std::cout << "Error Code: " << e.getErrorCode() << std::endl;
		std::cout << "SQLState: " << e.getSQLState() << std::endl;
	}
}

void MysqlPool::checkConnection() {
	std::lock_guard<std::mutex> gaurd(_mutex);
	int poolsize = _pool.size();

	auto currentTime = std::chrono::system_clock::now().time_since_epoch();
	long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();

	/*
	遍历连接池，判断其中每个连接的没操作的时间间隔，过长让它给mysql 发送一个信号防止断开
	*/
	for (int q = 0; q < poolsize; q++) {
		auto con = std::move(_pool.front());
		_pool.pop();

		//RALL 思想：取出的连接最终都需要放回去，在Defer 的析构中执行这个表达式
		Defer defer([this,&con]() {
			_pool.push(std::move(con));
			});

		if (timestamp - con->_lasta_oper_time < 100) {
			continue;
		}
		try {
			//发送一个简单的查询操作
			std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
			stmt->executeQuery("SELECT 1");
			con->_lasta_oper_time = timestamp;
			//std::cout << "the " << q << " connection is become alive ,cur is " << timestamp << std::endl;
		}
		catch (sql::SQLException& e) {
			//捕获到错误，说明有连接断开了
			std::cout << "error: the " << q << " connection failed :" << e.what() << std::endl;
			//重新连接:创建一个新连接替换原来的断开的连接
			sql::mysql::MySQL_Driver* driver = sql::mysql::get_driver_instance();
			auto* newcon = driver->connect(_url, _user, _passwd);
			newcon->setSchema(_schema);
			/*
			unique_ptr<T>::reset(T* ptr = nullptr)
			如果当前 unique_ptr 已经管理了一个对象，它会先调用 delete 销毁原对象；
			然后把内部指针换成你传入的 ptr；
			如果你传入的是 nullptr，就等于释放当前资源，让 unique_ptr 变为空。
			*/
			con->_con.reset(newcon);
			con->_lasta_oper_time = timestamp;
			std::cout << "new success a connection!..." << std::endl;
		}
	}
}

std::unique_ptr<SqlConnection> MysqlPool::getConnection() {
	//判断能不能取到，如果不能则挂起等待
	std::unique_lock<std::mutex> lock(_mutex);
	_cond.wait(lock, [this] {
		if (_stop) return true;
		return !_pool.empty();
		});
	if (_stop) return nullptr;
	//不为空，取出连接
	std::unique_ptr<SqlConnection> con(std::move(_pool.front()));
	_pool.pop();
	return con;
}

void MysqlPool::returnConnection(std::unique_ptr<SqlConnection> con) {
	std::unique_lock<std::mutex> lock(_mutex);
	if (_stop) return;
	_pool.push(std::move(con));
	_cond.notify_one();
}

void MysqlPool::Close() {
	_stop = true;
	_cond.notify_all();
}

MysqlPool::~MysqlPool() {
	std::lock_guard<std::mutex>lock(_mutex);
	while (!_pool.empty()) {
		_pool.pop();
	}
}
//---------------------------------------Dao------------------------------------------------

MysqlDao::MysqlDao() {
	auto& cfg = ConfigMgr::getInstance();
	string host = cfg["Mysql"]["Host"];
	string port = cfg["Mysql"]["Port"];
	string pwd = cfg["Mysql"]["Passwd"];
	string schema = cfg["Mysql"]["Schema"];
	string user = cfg["Mysql"]["User"];
	_sqlpool.reset(new MysqlPool(host +":"+ port, user, pwd, schema, 5));
}

MysqlDao::~MysqlDao() {
	_sqlpool->Close();
}
int MysqlDao::regUser(const string& name, const string& email, const string& pwd) {
	auto con = _sqlpool->getConnection();
	// 自动归还连接，无论函数如何退出
	Defer defer([this, &con]() {
		_sqlpool->returnConnection(std::move(con));
		});
	if (con == nullptr) return 0;
	try {
		// 准备调用存储过程
		std::unique_ptr<sql::PreparedStatement> stmt(
			con->_con->prepareStatement("CALL reg(?,?,?,@result)")
		);
		// 设置输入参数
		stmt->setString(1, name);
		stmt->setString(2, email);
		stmt->setString(3, pwd);

		// 执行存储过程
		stmt->execute();

		// 获取输出参数 @result
		std::unique_ptr<sql::Statement> stmtResult(con->_con->createStatement());
		std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));

		if (res->next()) {
			int result = res->getInt("result");
			std::cout << "Result: " << result << std::endl;
			return result;
		}

		return -1; // 没有返回结果，视为失败
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return -1;
	}
}
bool MysqlDao::CheckEmail(const std::string& name, const std::string& email) {
	auto con = _sqlpool->getConnection();
	Defer defer([this, &con]() {
		_sqlpool->returnConnection(std::move(con));
		});
	try {
		if (con == nullptr) return false;
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->_con->prepareStatement("SELECT email FROM user WHERE name = ?")
		);
		pstmt->setString(1, name);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if (res->next()) {
			std::string dbEmail = res->getString("email");
			std::cout << "Check Email: " << dbEmail << std::endl;
			return email == dbEmail;
		}
		// 用户不存在
		return false;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what() << std::endl
			<< " (MySQL error code: " << e.getErrorCode()<<std::endl
			<< " SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::UpdatePwd(const std::string& name, const std::string& pwd) {
	auto con = _sqlpool->getConnection();
	// 自动归还连接，无论函数如何退出
	Defer defer([this, &con]() {
		_sqlpool->returnConnection(std::move(con));
		});
	try {
		if (con == nullptr) return false;
		// 准备查询语句
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("UPDATE user SET pwd = ? WHERE name = ?"));
		// 绑定参数
		pstmt->setString(2, name);
		pstmt->setString(1, pwd);
		// 执行更新
		int updateCount = pstmt->executeUpdate();
		std::cout << "Updated rows: " << updateCount << std::endl;
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what() << std::endl;
		std::cerr << " (MySQL error code: " << e.getErrorCode() << std::endl;
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}


bool MysqlDao::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo) {
	std::cout << "email is :" << email << std::endl;
	std::cout << "ori pwd is :" << pwd << std::endl;
	auto con = _sqlpool->getConnection();
	if (!con) return false;
	Defer defer([this, &con]() {
		_sqlpool->returnConnection(std::move(con));
		});
	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->_con->prepareStatement("SELECT uid, name, pwd FROM user WHERE email = ?")
		);
		pstmt->setString(1, email);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if (!res->next()) {
			std::cout << "No such user with email: " << email << std::endl;
			return false;
		}
		std::string origin_pwd = res->getString("pwd");
		std::cout << "checkd pwd is :" << origin_pwd << std::endl;
		if (pwd != origin_pwd) {
			std::cout << "Password mismatch for email: " << email << std::endl;
			return false;
		}
		// 登录成功，填充用户信息
		userInfo.email = email;
		userInfo.name = res->getString("name");
		userInfo.uid = res->getInt("uid");
		userInfo.pwd = origin_pwd;
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what()
			<< " (MySQL error code: " << e.getErrorCode()
			<< ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}
