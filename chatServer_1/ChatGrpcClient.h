#pragma once
#include "const_.h"
#include "SingletonPtr.h"
#include <grpcpp/grpcpp.h> 
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <queue>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::AddFriendReq;
using message::AddFriendRsp;

using message::AuthFriendReq;
using message::AuthFriendRsp;

using message::GetChatServerRsp;
using message::LoginRsp;
using message::LoginReq;
using message::ChatService;

using message::TextChatMsgReq;
using message::TextChatMsgRsp;
using message::TextChatData;


// grpc 连接池
class ChatConPool {
private:
	std::queue<std::unique_ptr<ChatService::Stub>> connections_;
	size_t poolSize_;
	std::string host_;
	std::string port_;
	std::atomic<bool> b_stop_;

	std::mutex mutex_;
	std::condition_variable cond_;
public:
	ChatConPool(size_t poolSize, std::string host, std::string port);
	~ChatConPool();
	std::unique_ptr<ChatService::Stub> getConnection();
	void returnConnection(std::unique_ptr<ChatService::Stub> context);
	void Close();
};



class ChatGrpcClient:public SingletonPtr<ChatGrpcClient>{
	friend class SingletonPtr<ChatGrpcClient>;
private:
	ChatGrpcClient();
	std::unordered_map<std::string, std::unique_ptr<ChatConPool>> _pools;     /// chatServer 的名字， 连接池

public:
	~ChatGrpcClient();
	// 申请添加好友
	AddFriendRsp NotifyAddFriend(std::string server_ip, const AddFriendReq& req);
	// 添加好友的认证
	AuthFriendRsp NotifyAuthFriend(std::string server_ip, const AuthFriendReq& req);
	bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);
	//发送消息
	TextChatMsgRsp NotifyTextChatMsg(std::string server_ip, const TextChatMsgReq& req, const Json::Value& rtvalue);

	message::KillUserRsp NotifyKillUser(std::string server_ip,const message::KillUserReq& req);
};

