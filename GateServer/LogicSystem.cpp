#include "LogicSystem.h"
#include "HttpConnection.h"
#include "VerityGrpcClient.h"
#include "StatusGrpcClient.h"
#include "RedisMgr.h"
#include "MysqlMgr.h"


LogicSystem::~LogicSystem() {

}

//ע��һ�� get ����
void LogicSystem::RegGet(std::string url, httpHandler handler) {    // ·�ɵ�ַ���Ự����
	_get_handlers.insert(make_pair(url, handler));
}

void LogicSystem::RegPost(std::string url, httpHandler handler) {    // ·�ɵ�ַ���Ự����
	_post_handlers.insert(make_pair(url, handler));
}
//����һ�� get ����
bool LogicSystem::handleGet(std::string path, std::shared_ptr<HttpConnection> con) {//·�ɵ�ַ���Ự����ptr
	if (_get_handlers.find(path) == _get_handlers.end()) {
		return false;
	}
	_get_handlers[path](con);
	return true;
}

bool LogicSystem::handlePost(std::string path, std::shared_ptr<HttpConnection>con) {
	if (_post_handlers.find(path) == _post_handlers.end()) {
		return false;
	}
	_post_handlers[path](con);
	return true;
}

LogicSystem::LogicSystem() {
	// get �������
	RegGet("/get_test", [](std::shared_ptr<HttpConnection> conncection) {
		//�� HTTP ��Ӧ�� _response.body() д���ַ��� "recive get_test req"
		beast::ostream(conncection->_response.body()) << "recive get_test req"<<std::endl;
		int i = 0;
		for (auto& elem : conncection->_get_params) {
			i++;
			beast::ostream(conncection->_response.body()) << "param: " << i << "  ----  key is: " << elem.first;
			beast::ostream(conncection->_response.body()) << ", " << " value is: " << elem.second << std::endl;
		}
		});

	//��ȡ��֤������
	RegPost("/get_varifycode", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;                   //���ڹ�����Ӧ json
		Json::Reader reader;                //json ������
		Json::Value src_root;               //�洢�����������
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {               //����ʧ��
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::ERROR_JSON;
			std::string jsonstr = root.toStyledString();          //���л�Ϊ�ַ�����tcp ��������
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		//���� post �����е�email �ֶ�
		auto email = src_root["email"].asString();
		std::cout << "email is " << email << std::endl;
		/*
		 //��ע����֤����������
		 �˴���Ϊ ��ȡ��֤����Ŀͻ��ˣ�ͨ�� grpc ����֤���������󣬻�÷���ֵ
		*/
		GetVarifyRsp rps = VerityGrpcClient::getInstance().getVarityCode(email); 
		root["error"] = rps.error();
		root["email"] = src_root["email"];

		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	///ע��ģ�� �û�ע������
	RegPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {

		//std::cout << "inter there" << std::endl;

		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());  //�ͻ���http������
		std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::ERROR_JSON;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		//�Ȳ���redis��email��Ӧ����֤���Ƿ����
		std::string  varify_code;
		bool b_get_varify = RedisMgr::getInstance()->Get(CODEPREFIX + src_root["email"].asString(), varify_code);

		if (!b_get_varify) {
			std::cout << " get varify code expired" << std::endl;
			root["error"] = ErrorCodes::VarifyExpired;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		if (varify_code != src_root["varifycode"].asString()) {      //��֤�벻ƥ��
			std::cout << " varify code error" << std::endl;
			root["error"] = ErrorCodes::VarifyCodeErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		//�������ݿ��ж��û��Ƿ����
		int uid = MysqlMgr::getInstance()->regUser(src_root["user"].asString(), src_root["email"].asString(), src_root["passwd"].asString());
		
		std::cout << "uid id: " << uid << std::endl;
		if (uid == 0 || uid == -1) {
			std::cout << "user or email exist!..." << std::endl;
			root["error"] = ErrorCodes::UserExist;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		//std::cout << "inter there����1" << std::endl;

		root["error"] = 0;
		root["email"] = src_root["email"];
		root["uid"] = uid;
		root["user"] = src_root["user"].asString();
		root["passwd"] = src_root["passwd"].asString();
		root["confirm"] = src_root["confirm"].asString();
		root["varifycode"] = src_root["varifycode"].asString();
		//�ذ����ͻ���
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	//�һ�����ģ�� ������������
	RegPost("/reset_pwd", [](std::shared_ptr<HttpConnection> connection) {
		//json ��ʽ�������ַ���
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is : " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");

		Json::Value root;
		Json::Reader reader;       //json ������
		Json::Value src_root;      //�洢�������н���������json ����
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {                 //json ��������ֱ�ӷ��� ERROR_JSON�������
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::ERROR_JSON;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();
		auto name = src_root["user"].asString();
		auto pwd = src_root["passwd"].asString();
		//�Ȳ���redis��email��Ӧ����֤���Ƿ����
		std::string  varify_code;
		bool b_get_varify = RedisMgr::getInstance()->Get(CODEPREFIX + src_root["email"].asString(), varify_code);
		//��֤�����
		if (!b_get_varify) {
			std::cout << " get varify code expired" << std::endl;
			root["error"] = ErrorCodes::VarifyExpired;        //��֤�����
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		//��֤�����
		if (varify_code != src_root["varifycode"].asString()) {
			std::cout << " varify code error" << std::endl;
			root["error"] = ErrorCodes::VarifyCodeErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		//��ѯ���ݿ��ж��û����������Ƿ�ƥ��
		bool email_valid = MysqlMgr::getInstance()->CheckEmail(name, email);
		if (!email_valid) {
			std::cout << " user email not match" << std::endl;
			root["error"] = ErrorCodes::EmailNotMatch;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		//��������Ϊ��������
		bool b_up = MysqlMgr::getInstance()->UpdatePwd(name, pwd);
		//���ݿ���´���
		if (!b_up) {
			std::cout << " update pwd failed" << std::endl;
			root["error"] = ErrorCodes::PasswdUpFailed;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		std::cout << "succeed to update password" << pwd << std::endl;
		root["error"] = 0;
		root["email"] = email;
		root["user"] = name;
		root["passwd"] = pwd;
		root["varifycode"] = src_root["varifycode"].asString();
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	//��¼ģ�� ��¼����
	RegPost("/user_login", [](std::shared_ptr<HttpConnection> connection) {
		auto str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is : " << str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(str, src_root);
		if (!parse_success) {                                                  //����JSON��������
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::ERROR_JSON;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		auto name = src_root["email"].asString();
		auto pwd = src_root["password"].asString();
		UserInfo userInfo;
		//��ѯ���ݿ��ж��û����������Ƿ�ƥ��
		bool pwd_valid = MysqlMgr::getInstance()->CheckPwd(name, pwd, userInfo);
		if (!pwd_valid) {                                                      //��ƥ�䣬��¼ʧ��
			std::cout << " user pwd not match" << std::endl;
			root["error"] = ErrorCodes::PasswdInvalid;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		//��ѯStatusServer�ҵ����ʵ������������Ϣ
		auto reply = StatusGrpcClient::getInstance()->GetChatServer(userInfo.uid);
		if (reply.error()) {
			std::cout << " grpc get chat server failed, error is " << reply.error() << std::endl;
			root["error"] = ErrorCodes::RPCGetFailed;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		std::cout << "succeed to load userinfo uid is " << userInfo.uid << std::endl;
		root["email"] = name;
		root["error"] = 0;
		root["Uid"] = userInfo.uid;
		root["Token"] = reply.token();
		root["Host"] = reply.host();
		root["Port"] = reply.port();
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});
}






