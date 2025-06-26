#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "const.h"
#include <boost/uuid/uuid.hpp>             // ���� UUID ����
#include <boost/uuid/uuid_generators.hpp>  // ���������� random_generator��
#include <boost/uuid/uuid_io.hpp>          // to_string �� I/O ֧��
#include "RedisMgr.h"
#include "Defer.h"


std::string generate_unique_string() {
	// ����UUID����
	boost::uuids::uuid uuid = boost::uuids::random_generator()();


	// ��UUIDת��Ϊ�ַ���
	std::string unique_string = to_string(uuid);

	return unique_string;
}


//��Ӧ grpc �ͻ��˵� GetChatServer ��������ʵ��
Status StatusServiceImpl::GetChatServer(ServerContext* context, const GetChatServerReq* request, GetChatServerRsp* reply)
{
	std::string prefix("status server has received :  ");
	const auto& server = getChatServer();                     //����ѹ����С�����������
	reply->set_host(server.host);
	reply->set_port(server.port);
	reply->set_error(ErrorCodes::SUCCESS);
	reply->set_token(generate_unique_string());
	insertToken(request->uid(), reply->token());    //��uid + token �洢����
	return Status::OK;
}

StatusServiceImpl::StatusServiceImpl()
{
	auto& cfg = ConfigMgr::getInstance();
	auto server_list = cfg["ChatServices"]["Name"];

	std::vector<std::string> words;
	std::stringstream ss(server_list);
	std::string word;
	while (std::getline(ss, word, ',')) {
		//std::cout << "server is: "<<word << std::endl;
		words.push_back(word);
	}

	for (auto& word : words) {
		if (cfg[word]["Name"].empty()) {
			continue;
		}
		ChatServer server;
		server.port = cfg[word]["Port"];
		server.host = cfg[word]["Host"];
		server.name = cfg[word]["Name"];

		_servers[server.name] = server;
	}
}

ChatServer StatusServiceImpl::getChatServer() {
	//ѡ����ǰʹ�������ٵ����������
	std::lock_guard<std::mutex> guard(_server_mtx);
	auto minServer = _servers.begin()->second;

	// ��ѯ֮ǰ�� reids ���Ϸֲ�ʽ��
	auto lock_key = LOCK_COUNT;
	auto identifier = RedisMgr::getInstance()->acquireLock(lock_key, LOCK_TIME_OUT, ACQUIRE_TIME_OUT);
	Defer defer2([this,identifier,lock_key]() {
		RedisMgr::getInstance()->releaseLock(lock_key, identifier);
		});


	auto count_str = RedisMgr::getInstance()->HGet(LOGIN_COUNT, minServer.name);
	if (count_str.empty()) {
		//������(�÷������ط���)��Ĭ������Ϊ���
		minServer.con_count = INT_MAX;
	}
	else minServer.con_count = std::stoi(count_str);
	// ʹ�÷�Χ����forѭ��
	for (auto& server : _servers) {
		
		std::cout << server.second.name << " service has " << RedisMgr::getInstance()->HGet(LOGIN_COUNT, server.second.name) << " connnect" << std::endl;
		if (server.second.name == minServer.name) continue;

		auto count_str = RedisMgr::getInstance()->HGet(LOGIN_COUNT, server.second.name);
		if (count_str.empty()) server.second.con_count = INT_MAX;
		else {
			server.second.con_count = std::stoi(count_str);
		}
		if (server.second.con_count < minServer.con_count) {
			minServer = server.second;
		}
	}
	std::cout << "use the min used server name is " << minServer.name << std::endl;
	return minServer;
}

//�����������¼��ѯ�Ľӿ�
Status StatusServiceImpl::Login(ServerContext* context, const LoginReq* request, LoginRsp* reply)
{
	auto uid = request->uid();
	auto token = request->token();

	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	std::string token_value = "";
	bool success = RedisMgr::getInstance()->Get(token_key, token_value);
	if (success) {
		reply->set_error(ErrorCodes::UidInvalid);
		return Status::OK;
	}

	if (token_value != token) {
		reply->set_error(ErrorCodes::TokenInvalid);
		return Status::OK;
	}
	reply->set_error(ErrorCodes::SUCCESS);
	reply->set_uid(uid);
	reply->set_token(token);
	return Status::OK;
}

void StatusServiceImpl::insertToken(int uid, std::string token)
{
	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	RedisMgr::getInstance()->Set(token_key, token);
}