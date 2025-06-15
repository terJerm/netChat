#include "ChatGrpcClient.h"
#include "ConfigMgr.h"
#include <vector>
#include "RedisMgr.h"
#include "MysqlMgr.h"

/// 连接池 -------------------------------------------------------

ChatConPool::ChatConPool(size_t poolSize, std::string host, std::string port) :
	poolSize_(poolSize), host_(host), port_(port), b_stop_(false) {
	for (size_t i = 0; i < poolSize_; ++i) {
		std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port,
			grpc::InsecureChannelCredentials());
		connections_.push(ChatService::NewStub(channel));
	}
}

ChatConPool::~ChatConPool() {
	std::lock_guard<std::mutex> lock(mutex_);
	Close();
	while (!connections_.empty()) {
		connections_.pop();
	}
}

std::unique_ptr<ChatService::Stub> ChatConPool::getConnection() {
	std::unique_lock<std::mutex> lock(mutex_);
	cond_.wait(lock, [this] {
		if (b_stop_) return true;
		return !connections_.empty();
		});
	//如果停止则直接返回空指针
	if (b_stop_) return  nullptr;
	auto context = std::move(connections_.front());
	connections_.pop();
	return context;
}

void ChatConPool::returnConnection(std::unique_ptr<ChatService::Stub> context) {
	std::lock_guard<std::mutex> lock(mutex_);
	if (b_stop_) return;
	connections_.push(std::move(context));
	cond_.notify_one();
}

void ChatConPool::Close() {
	b_stop_ = true;
	cond_.notify_all();
}


// client ----------------------------------------------------------------------

ChatGrpcClient::ChatGrpcClient() {
	auto server_list = (*ConfigMgr::getInstance())["PeerServer"]["Servers"];
	std::vector<std::string> words;

	std::stringstream ss(server_list);
	std::string word;
	while (std::getline(ss, word, ',')) {
		words.push_back(word);
	}
	for (auto& word : words) {
		if ((*ConfigMgr::getInstance())[word]["Name"].empty()) continue;
		_pools[(*ConfigMgr::getInstance())[word]["Name"]] = std::make_unique<ChatConPool>(5,
			(*ConfigMgr::getInstance())[word]["Host"], (*ConfigMgr::getInstance())[word]["Port"]);
	}
}

ChatGrpcClient::~ChatGrpcClient() {

}

// 申请添加好友
AddFriendRsp ChatGrpcClient::NotifyAddFriend(std::string server_ip, const AddFriendReq& req) {
	AddFriendRsp rsp;
	Defer defer([&rsp, &req]() {
		rsp.set_error(ErrorCodes::Success);
		rsp.set_applyuid(req.applyuid());
		rsp.set_touid(req.touid());
		});

	auto find_iter = _pools.find(server_ip);
	if (find_iter == _pools.end()) {
		return rsp;
	}

	std::cout << "grpc 客户端调用" << std::endl;

	auto& pool = find_iter->second;
	ClientContext context;
	auto stub = pool->getConnection();
	Status status = stub->NotifyAddFriend(&context, req, &rsp);      /// 调用服务端接口函数
	Defer defercon([&stub, this, &pool]() {
		pool->returnConnection(std::move(stub));
		});

	if (!status.ok()) {
		rsp.set_error(ErrorCodes::RPCFailed);
		return rsp;
	}

	return rsp;
}
// 添加好友的认证
AuthFriendRsp ChatGrpcClient::NotifyAuthFriend(std::string server_ip, const AuthFriendReq& req) {
	AuthFriendRsp rsp;
	rsp.set_error(ErrorCodes::Success);

	Defer defer([&rsp, &req]() {
		rsp.set_fromuid(req.fromuid());
		rsp.set_touid(req.touid());
		});

	auto find_iter = _pools.find(server_ip);
	if (find_iter == _pools.end()) {
		return rsp;
	}

	auto& pool = find_iter->second;
	ClientContext context;
	auto stub = pool->getConnection();
	Status status = stub->NotifyAuthFriend(&context, req, &rsp);
	Defer defercon([&stub, this, &pool]() {
		pool->returnConnection(std::move(stub));
		});

	if (!status.ok()) {
		rsp.set_error(ErrorCodes::RPCFailed);
		return rsp;
	}

	return rsp;
}

bool ChatGrpcClient::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo) {
	std::string info_str = "";
	bool b_base = RedisMgr::getInstance()->Get(base_key, info_str);
	if (b_base) {
		Json::Reader reader;
		Json::Value root;
		reader.parse(info_str, root);
		userinfo->uid = root["uid"].asInt();
		userinfo->name = root["name"].asString();
		userinfo->pwd = root["pwd"].asString();
		userinfo->email = root["email"].asString();
		userinfo->nick = root["nick"].asString();
		userinfo->desc = root["desc"].asString();
		userinfo->sex = root["sex"].asInt();
		userinfo->icon = root["icon"].asString();
		std::cout << "user login uid is  " << userinfo->uid << " name  is "
			<< userinfo->name << " pwd is " << userinfo->pwd << " email is " << userinfo->email << std::endl;
	}
	else {
		std::shared_ptr<UserInfo> user_info = nullptr;
		user_info = MysqlMgr::getInstance()->GetUser(uid);
		if (user_info == nullptr) {
			return false;
		}

		userinfo = user_info;

		Json::Value redis_root;
		redis_root["uid"] = uid;
		redis_root["pwd"] = userinfo->pwd;
		redis_root["name"] = userinfo->name;
		redis_root["email"] = userinfo->email;
		redis_root["nick"] = userinfo->nick;
		redis_root["desc"] = userinfo->desc;
		redis_root["sex"] = userinfo->sex;
		redis_root["icon"] = userinfo->icon;
		RedisMgr::getInstance()->Set(base_key, redis_root.toStyledString());
	}
	return true;
}
//发送消息
TextChatMsgRsp ChatGrpcClient::NotifyTextChatMsg(std::string server_ip, const TextChatMsgReq& req, const Json::Value& rtvalue) {
	TextChatMsgRsp rsp;
	rsp.set_error(ErrorCodes::Success);
	Defer defer([&rsp, &req]() {
		rsp.set_fromuid(req.fromuid());
		rsp.set_touid(req.touid());
		rsp.set_type(req.type());
		for (const auto& text_data : req.textmsgs()) {
			TextChatData* new_msg = rsp.add_textmsgs();
			new_msg->set_msgid(text_data.msgid());
			new_msg->set_msgcontent(text_data.msgcontent());
		}

		});
	auto find_iter = _pools.find(server_ip);
	if (find_iter == _pools.end()) {
		return rsp;
	}

	auto& pool = find_iter->second;
	ClientContext context;
	auto stub = pool->getConnection();
	Status status = stub->NotifyTextChatMsg(&context, req, &rsp);
	Defer defercon([&stub, this, &pool]() {
		pool->returnConnection(std::move(stub));
		});

	if (!status.ok()) {
		rsp.set_error(ErrorCodes::RPCFailed);
		return rsp;
	}

	return rsp;

}

message::KillUserRsp ChatGrpcClient::NotifyKillUser(std::string server_ip, const message::KillUserReq& req)
{
	message::KillUserRsp rsp;
	Defer defer([&rsp,&req]() {
		rsp.set_error(ErrorCodes::Success);
		rsp.set_uid(req.uid());
		});
	auto find_iter = _pools.find(server_ip);
	if (find_iter == _pools.end()) {
		return rsp;
	}

	auto& pool = find_iter->second;
	ClientContext context;
	auto stub = pool->getConnection();
	Defer defer2([&stub,this,&pool]() {
		pool->returnConnection(std::move(stub));
		});
	Status status = stub->NotifyKillUser(&context,req,&rsp);   /// 调用接口

	if (!status.ok()) {
		rsp.set_error(ErrorCodes::RPCFailed);
		return rsp;
	}
	return rsp;
}
