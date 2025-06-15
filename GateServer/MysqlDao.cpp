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
			//���ӵ� mysql �����ݿ�   
			sql::Connection* con = driver->connect(_url, _user, _passwd);
			con->setSchema(_schema);

			auto currentTime = std::chrono::system_clock::now().time_since_epoch();
			long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
			//ѹ�����ӳ�
			_pool.push(std::make_unique<SqlConnection>(con, timestamp));
			std::cout << "the "<<q<<" connection from mysqlpools connected  success!..." << std::endl;
		}
		_check_thread = std::thread([this]() {                          //�������
			std::cout << "��̨�����߳����� ..." << std::endl;
			while (!_stop) {
				checkConnection();
				std::this_thread::sleep_for(std::chrono::seconds(60));  //һ���Ӹ�mysql ��һ���ź�
			}
			});
		_check_thread.detach();  //�ҵ���̨���ò���ϵͳ����
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
	�������ӳأ��ж�����ÿ�����ӵ�û������ʱ����������������mysql ����һ���źŷ�ֹ�Ͽ�
	*/
	for (int q = 0; q < poolsize; q++) {
		auto con = std::move(_pool.front());
		_pool.pop();

		//RALL ˼�룺ȡ�����������ն���Ҫ�Ż�ȥ����Defer ��������ִ��������ʽ
		Defer defer([this,&con]() {
			_pool.push(std::move(con));
			});

		if (timestamp - con->_lasta_oper_time < 100) {
			continue;
		}
		try {
			//����һ���򵥵Ĳ�ѯ����
			std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
			stmt->executeQuery("SELECT 1");
			con->_lasta_oper_time = timestamp;
			//std::cout << "the " << q << " connection is become alive ,cur is " << timestamp << std::endl;
		}
		catch (sql::SQLException& e) {
			//���񵽴���˵�������ӶϿ���
			std::cout << "error: the " << q << " connection failed :" << e.what() << std::endl;
			//��������:����һ���������滻ԭ���ĶϿ�������
			sql::mysql::MySQL_Driver* driver = sql::mysql::get_driver_instance();
			auto* newcon = driver->connect(_url, _user, _passwd);
			newcon->setSchema(_schema);
			/*
			unique_ptr<T>::reset(T* ptr = nullptr)
			�����ǰ unique_ptr �Ѿ�������һ�����������ȵ��� delete ����ԭ����
			Ȼ����ڲ�ָ�뻻���㴫��� ptr��
			����㴫����� nullptr���͵����ͷŵ�ǰ��Դ���� unique_ptr ��Ϊ�ա�
			*/
			con->_con.reset(newcon);
			con->_lasta_oper_time = timestamp;
			std::cout << "new success a connection!..." << std::endl;
		}
	}
}

std::unique_ptr<SqlConnection> MysqlPool::getConnection() {
	//�ж��ܲ���ȡ����������������ȴ�
	std::unique_lock<std::mutex> lock(_mutex);
	_cond.wait(lock, [this] {
		if (_stop) return true;
		return !_pool.empty();
		});
	if (_stop) return nullptr;
	//��Ϊ�գ�ȡ������
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
	// �Զ��黹���ӣ����ۺ�������˳�
	Defer defer([this, &con]() {
		_sqlpool->returnConnection(std::move(con));
		});
	if (con == nullptr) return 0;
	try {
		// ׼�����ô洢����
		std::unique_ptr<sql::PreparedStatement> stmt(
			con->_con->prepareStatement("CALL reg(?,?,?,@result)")
		);
		// �����������
		stmt->setString(1, name);
		stmt->setString(2, email);
		stmt->setString(3, pwd);

		// ִ�д洢����
		stmt->execute();

		// ��ȡ������� @result
		std::unique_ptr<sql::Statement> stmtResult(con->_con->createStatement());
		std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));

		if (res->next()) {
			int result = res->getInt("result");
			std::cout << "Result: " << result << std::endl;
			return result;
		}

		return -1; // û�з��ؽ������Ϊʧ��
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
		// �û�������
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
	// �Զ��黹���ӣ����ۺ�������˳�
	Defer defer([this, &con]() {
		_sqlpool->returnConnection(std::move(con));
		});
	try {
		if (con == nullptr) return false;
		// ׼����ѯ���
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("UPDATE user SET pwd = ? WHERE name = ?"));
		// �󶨲���
		pstmt->setString(2, name);
		pstmt->setString(1, pwd);
		// ִ�и���
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
		// ��¼�ɹ�������û���Ϣ
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
