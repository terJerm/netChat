#include "LogicSystem.h"
#include "HttpConnection.h"
#include "VerityGrpcClient.h"
#include "StatusGrpcClient.h"
#include "RedisMgr.h"
#include "MysqlMgr.h"


LogicSystem::~LogicSystem() {

}

//注册一个 get 请求
void LogicSystem::RegGet(std::string url, httpHandler handler) {    // 路由地址、会话对象
	_get_handlers.insert(make_pair(url, handler));
}

void LogicSystem::RegPost(std::string url, httpHandler handler) {    // 路由地址、会话对象
	_post_handlers.insert(make_pair(url, handler));
}
//处理一个 get 请求
bool LogicSystem::handleGet(std::string path, std::shared_ptr<HttpConnection> con) {//路由地址、会话对象ptr
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
	// get 请求测试
	RegGet("/get_test", [](std::shared_ptr<HttpConnection> conncection) {
		//向 HTTP 响应体 _response.body() 写入字符串 "recive get_test req"
		beast::ostream(conncection->_response.body()) << "recive get_test req"<<std::endl;
		int i = 0;
		for (auto& elem : conncection->_get_params) {
			i++;
			beast::ostream(conncection->_response.body()) << "param: " << i << "  ----  key is: " << elem.first;
			beast::ostream(conncection->_response.body()) << ", " << " value is: " << elem.second << std::endl;
		}
		});

	//获取验证码请求
	RegPost("/get_varifycode", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;                   //用于构造响应 json
		Json::Reader reader;                //json 解析器
		Json::Value src_root;               //存储解析后的数据
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {               //解析失败
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::ERROR_JSON;
			std::string jsonstr = root.toStyledString();          //序列化为字符流（tcp 传输流）
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		//解析 post 请求中的email 字段
		auto email = src_root["email"].asString();
		std::cout << "email is " << email << std::endl;
		/*
		 //给注册验证服务发送请求
		 此处作为 获取验证服务的客户端，通过 grpc 给验证服务发送请求，获得返回值
		*/
		GetVarifyRsp rps = VerityGrpcClient::getInstance().getVarityCode(email); 
		root["error"] = rps.error();
		root["email"] = src_root["email"];

		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	///注册模块 用户注册请求
	RegPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {

		//std::cout << "inter there" << std::endl;

		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());  //客户端http请求体
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
		//先查找redis中email对应的验证码是否合理
		std::string  varify_code;
		bool b_get_varify = RedisMgr::getInstance()->Get(CODEPREFIX + src_root["email"].asString(), varify_code);

		if (!b_get_varify) {
			std::cout << " get varify code expired" << std::endl;
			root["error"] = ErrorCodes::VarifyExpired;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		if (varify_code != src_root["varifycode"].asString()) {      //验证码不匹配
			std::cout << " varify code error" << std::endl;
			root["error"] = ErrorCodes::VarifyCodeErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		//查找数据库判断用户是否存在
		int uid = MysqlMgr::getInstance()->regUser(src_root["user"].asString(), src_root["email"].asString(), src_root["passwd"].asString());
		
		std::cout << "uid id: " << uid << std::endl;
		if (uid == 0 || uid == -1) {
			std::cout << "user or email exist!..." << std::endl;
			root["error"] = ErrorCodes::UserExist;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		//std::cout << "inter there——1" << std::endl;

		root["error"] = 0;
		root["email"] = src_root["email"];
		root["uid"] = uid;
		root["user"] = src_root["user"].asString();
		root["passwd"] = src_root["passwd"].asString();
		root["confirm"] = src_root["confirm"].asString();
		root["varifycode"] = src_root["varifycode"].asString();
		//回包给客户端
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	//找回密码模块 重置密码请求
	RegPost("/reset_pwd", [](std::shared_ptr<HttpConnection> connection) {
		//json 格式的请求字符串
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is : " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");

		Json::Value root;
		Json::Reader reader;       //json 解析器
		Json::Value src_root;      //存储从请求中解析出来的json 数据
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {                 //json 解析错误，直接返回 ERROR_JSON错误代码
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::ERROR_JSON;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();
		auto name = src_root["user"].asString();
		auto pwd = src_root["passwd"].asString();
		//先查找redis中email对应的验证码是否合理
		std::string  varify_code;
		bool b_get_varify = RedisMgr::getInstance()->Get(CODEPREFIX + src_root["email"].asString(), varify_code);
		//验证码过期
		if (!b_get_varify) {
			std::cout << " get varify code expired" << std::endl;
			root["error"] = ErrorCodes::VarifyExpired;        //验证码过期
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		//验证码错误
		if (varify_code != src_root["varifycode"].asString()) {
			std::cout << " varify code error" << std::endl;
			root["error"] = ErrorCodes::VarifyCodeErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		//查询数据库判断用户名和邮箱是否匹配
		bool email_valid = MysqlMgr::getInstance()->CheckEmail(name, email);
		if (!email_valid) {
			std::cout << " user email not match" << std::endl;
			root["error"] = ErrorCodes::EmailNotMatch;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		//更新密码为最新密码
		bool b_up = MysqlMgr::getInstance()->UpdatePwd(name, pwd);
		//数据库更新错误
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

	//登录模块 登录请求
	RegPost("/user_login", [](std::shared_ptr<HttpConnection> connection) {
		auto str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is : " << str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(str, src_root);
		if (!parse_success) {                                                  //请求JSON解析错误
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::ERROR_JSON;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		auto name = src_root["email"].asString();
		auto pwd = src_root["password"].asString();
		UserInfo userInfo;
		//查询数据库判断用户名和密码是否匹配
		bool pwd_valid = MysqlMgr::getInstance()->CheckPwd(name, pwd, userInfo);
		if (!pwd_valid) {                                                      //不匹配，登录失败
			std::cout << " user pwd not match" << std::endl;
			root["error"] = ErrorCodes::PasswdInvalid;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		//查询StatusServer找到合适的聊天服务器信息
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






