#include "MysqlDao.h"
#include "ConfigMgr.h"

SqlConnection::SqlConnection(sql::Connection* con, int64_t lasttime) :
	_con(con), _lasta_oper_time(lasttime) {

}

MysqlPool::MysqlPool(const std::string& url, const std::string& user, std::string& pass, const std::string& schema,
	int poolsize) :_url(url), _user(user), _passwd(pass), _schema(schema), _poolsize(poolsize), _stop(false) {
	try {
		for (int q = 0; q < _poolsize; q++) {
			sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
			//���ӵ� mysql �����ݿ�   
			sql::Connection* con = driver->connect(_url, _user, _passwd);
			con->setSchema(_schema);

			auto currentTime = std::chrono::system_clock::now().time_since_epoch();
			long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
			//ѹ�����ӳ�
			_pool.push(std::make_unique<SqlConnection>(con, timestamp));
			std::cout << "the " << q << " connection from mysqlpools connected  success!..." << std::endl;
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
		Defer defer([this, &con]() {
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
	auto cfg = ConfigMgr::getInstance();
	string host = (*cfg)["Mysql"]["Host"];
	string port = (*cfg)["Mysql"]["Port"];
	string pwd = (*cfg)["Mysql"]["Passwd"];
	string schema = (*cfg)["Mysql"]["Schema"];
	string user = (*cfg)["Mysql"]["User"];
	_sqlpool.reset(new MysqlPool(host + ":" + port, user, pwd, schema, 5));
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
			<< " (MySQL error code: " << e.getErrorCode() << std::endl
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

bool MysqlDao::addFriendApply(const int& selfuid, const int& useruid, const std::string& mess, const std::string& nick){
	auto con = _sqlpool->getConnection();
	if (con == nullptr) return false;

	Defer defer([this, &con]() {
		_sqlpool->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::PreparedStatement>pstmt(con->_con->prepareStatement("INSERT INTO friend_apply (from_uid, to_uid,message,nick,status) values (?,?,?,?,?) "
			"ON DUPLICATE KEY UPDATE from_uid = from_uid, to_uid = to_uid, message = message ,nick = nick,status = status"));
		pstmt->setInt(1, selfuid);
		pstmt->setInt(2, useruid);
		pstmt->setString(3, mess);
		pstmt->setString(4, nick);
		pstmt->setInt(5, 0);

		// ִ��
		int rowAffected = pstmt->executeUpdate();
		if (rowAffected < 0) return false;
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what() << std::endl;
		std::cerr << " (MySQL error code: " << e.getErrorCode() << std::endl;
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}

	return true;
}


std::shared_ptr<UserInfo> MysqlDao::GetUser(int uid)
{
	auto con = _sqlpool->getConnection();
	if (con == nullptr) {
		return nullptr;
	}

	Defer defer([this, &con]() {
		_sqlpool->returnConnection(std::move(con));
		});

	try {
		// ׼��SQL���
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE uid = ?"));
		pstmt->setInt(1, uid); // ��uid�滻Ϊ��Ҫ��ѯ��uid

		// ִ�в�ѯ
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		std::shared_ptr<UserInfo> user_ptr = nullptr;
		// ���������
		while (res->next()) {
			user_ptr.reset(new UserInfo);
			user_ptr->uid = res->getInt("uid");
			user_ptr->pwd = res->getString("pwd");
			user_ptr->email = res->getString("email");
			user_ptr->name = res->getString("name");
			user_ptr->nick = res->getString("nick");
			user_ptr->desc = res->getString("desc");
			user_ptr->sex = res->getInt("sex");
			user_ptr->icon = res->getString("icon");
			user_ptr->uid = uid;
			break;
		}
		return user_ptr;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return nullptr;
	}
}

std::shared_ptr<UserInfo> MysqlDao::GetUser(std::string name) {
	auto con = _sqlpool->getConnection();
	if (con == nullptr) {
		return nullptr;
	}

	Defer defer([this, &con]() {
		_sqlpool->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE name = ?"));
		pstmt->setString(1, name); 

		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		std::shared_ptr<UserInfo> user_ptr = nullptr;
		std::cout << "-------------------thie-------------------" << std::endl;
		
		while (res->next()) {
			user_ptr.reset(new UserInfo);
			user_ptr->pwd = res->getString("pwd");
			user_ptr->email = res->getString("email");
			user_ptr->name = res->getString("name");
			user_ptr->nick = res->getString("nick");
			user_ptr->desc = res->getString("desc");
			user_ptr->sex = res->getInt("sex");
			user_ptr->uid = res->getInt("uid");
			break;
		}
		return user_ptr;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return nullptr;
	}
}


bool MysqlDao::AuthFriendApply(int selfuid, int touid) {
	std::cout << "------------------------------------------selfuid is : " << selfuid << std::endl;
	std::cout << "------------------------------------------touid is ; " << touid << std::endl;
	auto con = _sqlpool->getConnection();
	if (con == nullptr) return false;
	Defer defer([this,&con]() {
		_sqlpool->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::PreparedStatement> ps(con->_con->prepareStatement("UPDATE friend_apply set status = 1 "
		"where from_uid= ? and to_uid = ?"));
		ps->setInt(1, touid);
		ps->setInt(2, selfuid);

		int row = ps->executeUpdate();
		if (row < 0) return false;
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::AddFriend(int selfuid, int touid) {
	auto con = _sqlpool->getConnection();
	if (con == nullptr) return false;
	Defer defer([this, &con]() {
		_sqlpool->returnConnection(std::move(con));
		});

	try {
		std::unique_ptr<sql::PreparedStatement> ps(
			con->_con->prepareStatement(
				"INSERT IGNORE INTO friend (uid1, uid2) VALUES (?, ?), (?, ?)"
			)
		);
		ps->setInt(1, std::min(selfuid, touid));
		ps->setInt(2, std::max(selfuid, touid));
		ps->setInt(3, std::max(selfuid, touid));
		ps->setInt(4, std::min(selfuid, touid));
		bool is = ps->execute();
		if (is) return true;
		return false;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::addMessageNode(const int& senderuid, const int& receiveruid, std::string& message, std::string& marking, bool& b_ip,int type) {

	auto con = _sqlpool->getConnection();
	if (con == nullptr || con->_con == nullptr) {
		std::cerr << "Failed to get a valid database connection." << std::endl;
		return false;
	}

	Defer defer([this, &con]() {
		_sqlpool->returnConnection(std::move(con));
		});

	try {
		// ���� session_id��ȷ��˳��һ��
		std::string session_id = std::to_string(std::min(senderuid, receiveruid)) + "_" + std::to_string(std::max(senderuid, receiveruid));
		// ���� session_id ����ֱ�
		int tableIndex = static_cast<int>(std::hash<std::string>{}(session_id) % 10);
		std::string tableName = "messages_" + std::to_string(tableIndex);

		std::cout << "session_id: " << session_id << std::endl;
		std::cout << "tableName: " << tableName << std::endl;

		// ׼�� SQL ���
		std::string sql = "INSERT INTO " + tableName +
			" (session_id, sender_id, receiver_id, content, msg_type, status, marking) "
			"VALUES (?, ?, ?, ?, ?, ?, ?)";
		std::unique_ptr<sql::PreparedStatement> ps(con->_con->prepareStatement(sql));

		// ���ò���
		ps->setString(1, session_id);
		ps->setInt(2, senderuid);
		ps->setInt(3, receiveruid);
		ps->setString(4, message);
		ps->setInt(5, type);             // msg_type = 1 (�ı�)
		ps->setInt(6, b_ip ? 0 : 1);  // status ���� b_ip ȷ����0-δ����1-�Ѷ�
		ps->setString(7, marking);

		// ִ�в����Ӱ�������
		int affected_rows = ps->executeUpdate();
		std::cout << "Affected Rows: " << affected_rows << std::endl;

		// ����Ƿ��о���
		if (ps->getWarnings()) {
			std::cerr << "MySQL Warning: " << ps->getWarnings()->getMessage() << std::endl;
		}

		return affected_rows > 0;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}

}

bool MysqlDao::getFriendListFromUid(int& uid, Json::Value& rtvalue)
{
	auto con = _sqlpool->getConnection();
	if (con == nullptr) return false;
	Defer defer([this, &con]() {
		_sqlpool->returnConnection(std::move(con));
		});
	try {
		// ��ѯ friend ���еĺ��� uid2
		std::unique_ptr<sql::PreparedStatement> ps(
			con->_con->prepareStatement(
				"SELECT uid2 FROM friend WHERE uid1 = ?"
			)
		);
		ps->setInt(1, uid);
		std::unique_ptr<sql::ResultSet> rs(ps->executeQuery());
		// ���û�к��ѣ����ؿ�����
		if (!rs->next()) {
			rtvalue["friends"] = Json::arrayValue;
			return true;
		}
		// ���� uid2 �б�
		std::vector<int> uid_list;
		do {
			uid_list.push_back(rs->getInt("uid2"));
		} while (rs->next());
		// ͨ�� uid2 �б��ѯ user ����Ϣ
		std::stringstream query;
		query << "SELECT * FROM user WHERE uid IN (";
		for (size_t i = 0; i < uid_list.size(); ++i) {
			if (i > 0) query << ",";
			query << uid_list[i];
		}
		query << ")";
		std::unique_ptr<sql::PreparedStatement> ps_users(
			con->_con->prepareStatement(query.str())
		);
		std::unique_ptr<sql::ResultSet> rs_users(ps_users->executeQuery());
		// ��������Ϣ���� JSON
		Json::Value friends(Json::arrayValue);
		while (rs_users->next()) {
			Json::Value friend_info;
			friend_info["uid"] = rs_users->getInt("uid");
			friend_info["name"] = rs_users->getString("name").asStdString();
			friend_info["email"] = rs_users->getString("email").asStdString();
			friend_info["nick"] = rs_users->getString("nick").asStdString();
			friend_info["desc"] = rs_users->getString("desc").asStdString();
			friend_info["sex"] = rs_users->getInt("sex");
			friend_info["icon"] = rs_users->getString("icon").asStdString();
			friends.append(friend_info);
		}
		rtvalue["friends"] = friends;
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}

	return true;
}

bool MysqlDao::getFriendRequestListFromUid(int& uid, Json::Value& rtvalue)
{
	auto con = _sqlpool->getConnection();
	if (con == nullptr) return false;

	Defer defer([this, &con]() {
		_sqlpool->returnConnection(std::move(con));
		});

	try {
		// ��ѯ friend_apply �������� to_uid = uid �ļ�¼
		std::unique_ptr<sql::PreparedStatement> ps(
			con->_con->prepareStatement(
				"SELECT from_uid, message, status FROM friend_apply WHERE to_uid = ?"
			)
		);
		ps->setInt(1, uid);
		std::unique_ptr<sql::ResultSet> rs(ps->executeQuery());

		// ���û�����룬���ؿ�����
		Json::Value requests(Json::arrayValue);
		if (!rs->next()) {
			rtvalue["requests"] = requests;
			return true;
		}

		// ������������
		do {
			int from_uid = rs->getInt("from_uid");
			std::string message = rs->getString("message").asStdString();
			int status = rs->getInt("status");

			// ��ȡ�����˵��û���Ϣ
			std::unique_ptr<sql::PreparedStatement> ps_user(
				con->_con->prepareStatement(
					"SELECT uid, name, email, nick, `desc`, sex, icon FROM user WHERE uid = ?"
				)
			);
			ps_user->setInt(1, from_uid);
			std::unique_ptr<sql::ResultSet> rs_user(ps_user->executeQuery());

			if (rs_user->next()) {
				Json::Value request;
				request["from_uid"] = from_uid;
				request["message"] = message;
				request["status"] = status;

				// �������û���Ϣ
				request["user_info"]["uid"] = rs_user->getInt("uid");
				request["user_info"]["name"] = rs_user->getString("name").asStdString();
				request["user_info"]["email"] = rs_user->getString("email").asStdString();
				request["user_info"]["nick"] = rs_user->getString("nick").asStdString();
				request["user_info"]["desc"] = rs_user->getString("desc").asStdString();
				request["user_info"]["sex"] = rs_user->getInt("sex");
				request["user_info"]["icon"] = rs_user->getString("icon").asStdString();
				requests.append(request);
			}
		} while (rs->next());

		// ������������Ϣ���� JSON
		rtvalue["requests"] = requests;
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::getMessageListFromUid(int& selfuid, int& useruid, Json::Value& rtvalue)
{
	auto con = _sqlpool->getConnection();
	if (con == nullptr) return false;

	Defer defer([this, &con]() {
		_sqlpool->returnConnection(std::move(con));
		});

	try {
		// ���� session_id��ȷ��˳��һ��
		std::string session_id = std::to_string(std::min(selfuid, useruid)) + "_" + std::to_string(std::max(selfuid, useruid));
		// ���� session_id ����ֱ�
		int tableIndex = static_cast<int>(std::hash<std::string>{}(session_id) % 10);
		std::string tableName = "messages_" + std::to_string(tableIndex);

		std::cout << "session_id: " << session_id << std::endl;
		std::cout << "tableName: " << tableName << std::endl;

		// ׼�� SQL ��ѯ���
		std::unique_ptr<sql::PreparedStatement> stmt(con->_con->prepareStatement(
			"SELECT id, session_id, sender_id, receiver_id, content, msg_type, status, marking "
			"FROM " + tableName + " WHERE session_id = ? ORDER BY id DESC LIMIT 20"
		));
		stmt->setString(1, session_id);
		std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
		std::vector<Json::Value> messages;

		while (res->next()) {
			Json::Value msg;
			msg["id"] = res->getInt("id");
			msg["session_id"] = res->getString("session_id").asStdString();
			msg["sender_id"] = res->getInt("sender_id");
			msg["receiver_id"] = res->getInt("receiver_id");
			msg["content"] = res->getString("content").asStdString();
			msg["msg_type"] = res->getInt("msg_type");
			msg["status"] = res->getInt("status");
			msg["marking"] = res->getString("marking").asStdString();
			messages.push_back(msg);
		}

		// ��ת˳��
		std::reverse(messages.begin(), messages.end());

		// ��䵽 rtvalue["message"]
		for (const auto& m : messages) {
			rtvalue["message"].append(m);
		}

		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::getMoreMessageListFromUid(int& selfuid, int& touid, std::string& marking, Json::Value& rtvalue)
{
	auto con = _sqlpool->getConnection();
	if (con == nullptr) return false;

	Defer defer([this, &con]() {
		_sqlpool->returnConnection(std::move(con));
		});

	try {
		std::string session_id = std::to_string(std::min(selfuid, touid)) + "_" + std::to_string(std::max(selfuid, touid));
		int tableIndex = static_cast<int>(std::hash<std::string>{}(session_id) % 10);
		std::string tableName = "messages_" + std::to_string(tableIndex);

		// ��һ�����ҳ���Ƕ�Ӧ�� id
		int base_id = -1;
		{
			std::unique_ptr<sql::PreparedStatement> stmt(con->_con->prepareStatement(
				"SELECT id FROM " + tableName + " WHERE session_id = ? AND marking = ? LIMIT 1"
			));
			stmt->setString(1, session_id);
			stmt->setString(2, marking);
			std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
			if (res->next()) {
				base_id = res->getInt("id");
			}
			else {
				// û�ҵ��ñ�ǣ�ֱ�ӷ��ؿս��
				return true;
			}
		}
		// �ڶ�����ȡ base_id ֮ǰ�ļ�¼����� 20 ��
		std::unique_ptr<sql::PreparedStatement> stmt(con->_con->prepareStatement(
			"SELECT id, session_id, sender_id, receiver_id, content, msg_type, status, marking "
			"FROM " + tableName + " WHERE session_id = ? AND id < ? ORDER BY id DESC LIMIT 20"
		));
		stmt->setString(1, session_id);
		stmt->setInt(2, base_id);

		std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
		std::vector<Json::Value> messages;

		while (res->next()) {
			Json::Value msg;
			msg["id"] = res->getInt("id");
			msg["session_id"] = res->getString("session_id").asStdString();
			msg["sender_id"] = res->getInt("sender_id");
			msg["receiver_id"] = res->getInt("receiver_id");
			msg["content"] = res->getString("content").asStdString();
			msg["msg_type"] = res->getInt("msg_type");
			msg["status"] = res->getInt("status");
			msg["marking"] = res->getString("marking").asStdString();
			messages.push_back(msg);
		}

		// ��תΪ����
		std::reverse(messages.begin(), messages.end());
		for (const auto& m : messages) {
			rtvalue["message"].append(m);
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what()
			<< " (MySQL error code: " << e.getErrorCode()
			<< ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

