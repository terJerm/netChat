#pragma once

#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <mutex>
#include "const_.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using message::AddFriendReq;
using message::AddFriendRsp;

using message::AuthFriendReq;
using message::AuthFriendRsp;

using message::ChatService;
using message::TextChatMsgReq;
using message::TextChatMsgRsp;
using message::TextChatData;

class CServer;


/// <summary>
/// 作为 grpc 服务端 的实现
/// </summary>
class ChatServiceImpl final :public ChatService::Service{
	// 实现 charServerclient 中调用的接口
public:
	ChatServiceImpl();
	Status NotifyAddFriend(ServerContext* context, const AddFriendReq* request,
		AddFriendRsp* reply) override;

	Status NotifyAuthFriend(ServerContext* context,
		const AuthFriendReq* request, AuthFriendRsp* response) override;

	Status NotifyTextChatMsg(::grpc::ServerContext* context,
		const TextChatMsgReq* request, TextChatMsgRsp* response) override;

	bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);

	::grpc::Status NotifyKillUser(::grpc::ServerContext* context, const ::message::KillUserReq* request, 
		::message::KillUserRsp* response);

	void RegisterServer(std::shared_ptr<CServer> pServer);
private:
	std::shared_ptr<CServer> _p_server;
};

