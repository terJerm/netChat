#include "ChatServiceImpl.h"
#include "UserMgr.h"
#include "CSession.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "MysqlMgr.h"
#include "RedisMgr.h"
#include "CServer.h"

ChatServiceImpl::ChatServiceImpl() {

}
Status ChatServiceImpl::NotifyAddFriend(ServerContext* context, const AddFriendReq* request,AddFriendRsp* reply)  {

    std::cout << "服务器 grpc 调用" << std::endl;

    //查找用户是否在本服务器
    auto touid = request->touid();
    auto session = UserMgr::getInstance()->GetSession(touid);

    Defer defer([request, reply]() {
        reply->set_error(ErrorCodes::Success);
        reply->set_applyuid(request->applyuid());
        reply->set_touid(request->touid());
        });

    //用户不在内存中则直接返回
    if (session == nullptr) {
        return Status::OK;
    }

    //在内存中则直接发送通知对方
    Json::Value  rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["applyuid"] = request->applyuid();
    rtvalue["name"] = request->name();
    rtvalue["desc"] = request->desc();
    rtvalue["icon"] = request->icon();
    rtvalue["sex"] = request->sex();
    rtvalue["nick"] = request->nick();

    std::string return_str = rtvalue.toStyledString();

    session->Send(return_str, ID_NOTIFY_ADD_FRIEND_REQ);

    return Status::OK;
}

Status ChatServiceImpl::NotifyAuthFriend(ServerContext* context,
	const AuthFriendReq* request, AuthFriendRsp* response)  {

    auto selfuid = request->touid();          /// 自己的 uid
    auto useruid = request->fromuid();        /// 这个才是对方的uid

    Defer defer([request, response]() {
        response->set_error(ErrorCodes::Success);
        response->set_fromuid(request->fromuid());
        response->set_touid(request->touid());
        });

    auto session = UserMgr::getInstance()->GetSession(selfuid);
    if (session == nullptr) return Status::OK;

    std::string base_key = USER_BASE_INFO + std::to_string(useruid); // 查找对方的信息
    auto user_info = std::make_shared<UserInfo>();
    bool b_info = GetBaseInfo(base_key, useruid, user_info);

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	if (b_info) {
		rtvalue["uid"] = user_info->uid;
		rtvalue["name"] = user_info->name;
		rtvalue["email"] = user_info->email;
		rtvalue["nick"] = user_info->nick;
		rtvalue["desc"] = user_info->desc;
		rtvalue["sex"] = user_info->sex;
		rtvalue["icon"] = user_info->icon;
	}else rtvalue["error"] = ErrorCodes::UidInvalid;

	std::string return_str = rtvalue.toStyledString();
	session->Send(return_str, ID_NOTIFY_AUTH_FRIEND_REQ);
	return Status::OK;


	return Status::OK;
}

Status ChatServiceImpl::NotifyTextChatMsg(::grpc::ServerContext* context,
	const TextChatMsgReq* request, TextChatMsgRsp* response)  {

		auto touid = request->touid();
		auto session = UserMgr::getInstance()->GetSession(touid);
		response->set_error(ErrorCodes::Success);

		if (session == nullptr) {
			return Status::OK;
		}

		Json::Value  rtvalue;
		rtvalue["error"] = ErrorCodes::Success;
		rtvalue["selfuid"] = request->fromuid();
		rtvalue["touid"] = request->touid();
		rtvalue["type"] = request->type();

		for (auto& msg : request->textmsgs()) {
			rtvalue["message"] = msg.msgcontent();
			rtvalue["marking"] = msg.msgid();
			
		}
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_NOTIFY_TEXT_CHAT_MSG_REQ);
		return Status::OK;
}

bool ChatServiceImpl::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo) {
	
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

::grpc::Status ChatServiceImpl::NotifyKillUser(::grpc::ServerContext* context, const::message::KillUserReq* request, 
	::message::KillUserRsp* response)
{
	// 在这里不需要在给 redis 加锁了，以为在grpc客户端已经加过锁了

	// 查找用户是否再本服务器
	auto uid = request->uid();
	auto session = UserMgr::getInstance()->GetSession(uid);

	Defer defer([request, response]() {
		response->set_error(ErrorCodes::Success);
		response->set_uid(request->uid());
		});

	// 用户不存在则直接返回
	if (session == nullptr) {
		return Status::OK;
	}
	// 再内存中则直接发送离线通知
	session->NotifyOffline(uid);
	// 清除旧连接
	_p_server->ClearSession(session->GetSessionId());

	return Status::OK;
}

void ChatServiceImpl::RegisterServer(std::shared_ptr<CServer> pServer)
{
	_p_server = pServer;
}
