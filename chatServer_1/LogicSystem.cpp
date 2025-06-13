#include "LogicSystem.h"
#include "MsgNode.h"
#include "CSession.h"
#include "CServer.h"
#include "const_.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "MysqlMgr.h"
#include "StatusGrpcClient.h"
#include "RedisMgr.h"
#include "UserMgr.h"
#include "ChatGrpcClient.h"


LogicSystem::LogicSystem():_stop(false),_p_server(nullptr) {
	RegisterCallBacks();
	_work_thread = std::thread(&LogicSystem::work, this);
}

LogicSystem::~LogicSystem() {
	stop();
}
void LogicSystem::work() {
	while (true) {
		std::unique_lock<std::mutex> lock(_mutex);
		_cond.wait(lock, [this]() {
			return !_msg_que.empty() || _stop; //不为空或者退出时往下走，否则挂起
			});
		if (_stop)break;
		if (_msg_que.empty()) continue;

		auto msgnode = _msg_que.front();
		_msg_que.pop();
		lock.unlock();

		if (!msgnode || !msgnode->_recvnode) {
			std::cerr << "[LogicSystem] warning: empty msg_node or recvnode" << std::endl;
			continue;
		}
		std::cout << "recv_msg id is " << msgnode->_recvnode->_msg_id << std::endl;
		auto it = _fun_call_back.find(msgnode->_recvnode->_msg_id);
		//通过 ReqId 调用回调
		if (it != _fun_call_back.end()) {
			it->second(msgnode->_session,
				msgnode->_recvnode->_msg_id,
				std::string(msgnode->_recvnode->_data, msgnode->_recvnode->_cur_len));
		}
		else {
			std::cout << "msg id [" << msgnode->_recvnode->_msg_id << "] handler not found" << std::endl;
		}
	}
}
void LogicSystem::RegisterCallBacks() {

    //客户登录到聊天服务器 ReqId = 1005 
    _fun_call_back[MSG_CHAT_LOGIN] = [this](std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data) {

        Json::Reader reader;
        Json::Value root;
        reader.parse(msg_data, root);
        auto uid = root["uid"].asInt();
        auto token = root["token"].asString();
        std::cout << "user login uid is  " << uid << " user token  is "
            << token << std::endl;

        Json::Value  rtvalue;
        Defer defer([this, &rtvalue, session]() {
            std::string return_str = rtvalue.toStyledString();
            session->Send(return_str, MSG_CHAT_LOGIN_RSP);
            });

        //从redis获取用户token是否正确
        std::string uid_str = std::to_string(uid);
        std::string token_key = USERTOKENPREFIX + uid_str;
        std::string token_value = "";
        bool success = RedisMgr::getInstance()->Get(token_key, token_value);
        if (!success) {
            rtvalue["error"] = ErrorCodes::UidInvalid;
            return;
        }

        if (token_value != token) {
            rtvalue["error"] = ErrorCodes::TokenInvalid;
            return;
        }

        rtvalue["error"] = ErrorCodes::Success;

        // 登录时添加分布式锁，
        auto lock_key = LOCK_PREFIX + uid_str;
        auto identifier = RedisMgr::getInstance()->acquireLock(lock_key, LOCK_TIME_OUT, ACQUIRE_TIME_OUT);
        // 结束时解锁
        Defer defer2([this, identifier, lock_key]() {
            RedisMgr::getInstance()->releaseLock(lock_key, identifier);
            });

        std::string uid_ip_value = "";
        auto uid_ip_key = USERIPPREFIX + uid_str;
        // 查找原来 redis 有无该账号，有就需要通知下线
        bool b_ip = RedisMgr::getInstance()->Get(uid_ip_key, uid_ip_value);
        if (b_ip) {
            auto self_name = ConfigMgr::getInstance()->GetValue("selfServer", "Name");
            if (uid_ip_value == self_name) {
                // 原来的账号也是登录在本服务器上面的，淡服务器踢人

                // 查找旧的连接
                auto old_session = UserMgr::getInstance()->GetSession(uid);
                // 发送踢人逻辑
                if (old_session) {
                    old_session->NotifyOffline(uid);   /// 异地登陆，通知老客户端下线
                    //清除旧连接
                    _p_server->ClearSession(old_session->GetSessionId());
                }
            }
            else {
                // 对方不在本服务器上，则 grpc 通知其他服务器踢人 
                message::KillUserReq kill_req;
                kill_req.set_uid(uid);
                ChatGrpcClient::getInstance()->NotifyKillUser(uid_ip_value, kill_req);
            }
        }






        std::string base_key = USER_BASE_INFO + uid_str;
        auto user_info = std::make_shared<UserInfo>();
        bool b_base = GetBaseInfo(base_key, uid, user_info);
        if (!b_base) {
            rtvalue["error"] = ErrorCodes::UidInvalid;
            return;
        }
        rtvalue["uid"] = uid;
        rtvalue["pwd"] = user_info->pwd;
        rtvalue["name"] = user_info->name;
        rtvalue["email"] = user_info->email;
        rtvalue["nick"] = user_info->nick;
        rtvalue["desc"] = user_info->desc;
        rtvalue["sex"] = user_info->sex;
        rtvalue["icon"] = user_info->icon;
        ///----------------------------------------------------------------------------------------------------------------
        rtvalue["token"] = token;

        //从数据库获取申请列表

        //获取好友列表

        auto server_name = ConfigMgr::getInstance()->GetValue("selfServer", "Name");
        //将登录数量增加
        auto rd_res = RedisMgr::getInstance()->HGet(LOGIN_COUNT, server_name);
        std::cout << "rd_res is: " << rd_res << std::endl;
        int count = 0;
        if (!rd_res.empty()) {
            count = std::stoi(rd_res);
            std::cout << "count is: " << rd_res << std::endl;
        }

        count++;
        // 将更新后的 count 写入到 redis  中
        auto count_str = std::to_string(count);
        RedisMgr::getInstance()->HSet(LOGIN_COUNT, server_name, count_str);

        //session绑定用户uid
        session->SetUserId(uid);

        //为用户设置登录ip server的名字
        std::string  ipkey = USERIPPREFIX + uid_str;
        RedisMgr::getInstance()->Set(ipkey, server_name);

        //uid和session绑定管理,方便以后踢人操作
        UserMgr::getInstance()->SetUserSession(uid, session);
        std::string uid_session_key = USER_SESSION_PREFIX + uid_str;
        RedisMgr::getInstance()->Set(uid_session_key,session->GetSessionId());

        return;
        };

    // 查找朋友
    _fun_call_back[ID_SEARCH_USER_REQ] = [this](std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data) {
        Json::Reader reader;
        Json::Value root;
        reader.parse(msg_data, root);
        auto uid_str = root["uid"].asString();
        std::cout << "user SearchInfo uid is: " << uid_str << std::endl;

        Json::Value rtvalue;
        Defer deder([this, &rtvalue, session]() {
            std::string return_str = rtvalue.toStyledString();
            session->Send(return_str, ID_SEARCH_USER_RSP);
            });

        bool digit = isPureDigit(uid_str);
        if (digit) {
            GetUserByUid(uid_str, rtvalue);     // 纯数字则按uid查找
        }
        else {
            GetUserByName(uid_str, rtvalue);    // 不是纯数字则按名称查找
        }

        };

    // 好友申请请求
    _fun_call_back[ID_ADD_FRIEND_REQ] = [this](std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data) {
        Json::Reader reader;
        Json::Value root;
        reader.parse(msg_data, root);

        auto selfname = root["selfname"].asString();
        auto selfuid = root["selfuid"].asInt();
        auto useruid = root["touid"].asInt();
        auto message = root["message"].asString();
        auto nick = root["nick"].asString();
        std::cout << "self name is : " << selfname << " self uid is : " << selfuid << "user uid is : " << useruid
            << " message is : " << message << " nick is : " << nick << std::endl;

        Json::Value  rtvalue;
        rtvalue["error"] = ErrorCodes::Success;
        Defer defer([this, &rtvalue, session]() {
            std::string return_str = rtvalue.toStyledString();
            session->Send(return_str, ID_ADD_FRIEND_RSP);
            });

        // 先将该条申请数据更新至数据库
        MysqlMgr::getInstance()->addFriendApply(selfuid,useruid,message,nick);

        // 通过 redis 查找对方所在的 ChatServer
        auto to_str = std::to_string(useruid);
        auto to_ip_key = USERIPPREFIX + to_str;
        std::string to_ip_value = "";
        bool b_ip = RedisMgr::getInstance()->Get(to_ip_key, to_ip_value);
        std::cout << "对方聊天服务器的名字为：" << to_ip_value << std::endl;
        if (!b_ip) {
            std::cout << "-----------the user is not in redis,its not online...-----------" << std::endl;
            return;
        }
        //对方在线的情况
        std::cout << "---------- the user is online...-----------" << std::endl;

        auto cfg = ConfigMgr::getInstance();
        auto self_name = (*cfg)["selfServer"]["Name"];

        if (to_ip_value == self_name) {     // 如果是在同一个服务器上，则直接发送，无需跨服通知
            std::cout << "-----the user with self in the serice...-----" << std::endl;
            auto session = UserMgr::getInstance()->GetSession(useruid);
            if (session) {
                //在内存中则直接发送通知对方
                Json::Value  notify;
                notify["error"] = ErrorCodes::Success;
                notify["applyuid"] = selfuid;
                notify["name"] = selfname;
                notify["desc"] = message;
                notify["icon"] = "";                              // waiting..........
                notify["sex"] = "";
                notify["nick"] = "";
                std::string return_str = notify.toStyledString();
                session->Send(return_str, ID_NOTIFY_ADD_FRIEND_REQ);
            }
            return;
        }

        // 不在同一服务器上，需要跨服通知
        std::cout << "-----the user is not in the serice...-----" << std::endl;

        std::string base_key = USER_BASE_INFO + std::to_string(selfuid);        // 获取自身的一些信息一并发送给另一个服务器
        auto apply_info = std::make_shared<UserInfo>();
        bool b_info = GetBaseInfo(base_key, selfuid, apply_info);

        AddFriendReq add_req;                 // grpc 跨服调用
        add_req.set_applyuid(selfuid);
        add_req.set_touid(useruid);
        add_req.set_name(selfname);
        add_req.set_desc(message);
        if (b_info) {
            add_req.set_icon(apply_info->icon);
            add_req.set_sex(apply_info->sex);
            add_req.set_nick(apply_info->nick);
        }
        //发送通知
        std::cout << "给对应的聊天服务器发送grpc 请求" << std::endl;
        ChatGrpcClient::getInstance()->NotifyAddFriend(to_ip_value, add_req);  // 服务器的名字 chatServer_1 , AddFriendReq
        
        };

    //  同意对方好友申请 （对方发来的好友请求，我同意）
    _fun_call_back[ID_AUTH_FRIEND_REQ] = [this](std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data) {
        Json::Reader reader;
        Json::Value root;
        reader.parse(msg_data, root);

        auto fromuid = root["fromuid"].asInt();
        auto touid = root["touid"].asInt();
        std::cout << "--------self uid is : " << fromuid << std::endl;
        std::cout << "--------to uid is : " << touid << std::endl;

        Json::Value  rtvalue;
        rtvalue["error"] = ErrorCodes::Success;
        auto user_info = std::make_shared<UserInfo>();

        std::string base_key = USER_BASE_INFO + std::to_string(touid);  // 返回自己客户端对方的信息
        bool b_info = GetBaseInfo(base_key, touid, user_info);
        if (b_info) {
            rtvalue["name"] = user_info->name;
            rtvalue["nick"] = user_info->nick;
            rtvalue["icon"] = user_info->icon;
            rtvalue["sex"] = user_info->sex;
            rtvalue["uid"] = touid;
            rtvalue["email"] = user_info->email;
            rtvalue["desc"] = user_info->desc;
        }
        else rtvalue["error"] = ErrorCodes::UidInvalid;

        Defer defer([this, &rtvalue, session]() {
            std::string return_str = rtvalue.toStyledString();
            session->Send(return_str, ID_AUTH_FRIEND_RSP);
            });

        // 更新数据库 好友关系
        MysqlMgr::getInstance()->AuthFriendApply(fromuid, touid);
        MysqlMgr::getInstance()->AddFriend(fromuid, touid);

        // grpc 通知对方自己已经通过了好友申请,并把自己的信息发送给对方
        auto to_str = std::to_string(touid);
        auto to_ip_key = USERIPPREFIX + to_str;
        std::string to_ip_value = "";
        bool b_ip = RedisMgr::getInstance()->Get(to_ip_key, to_ip_value);   //  查找对方所在的服务器名字
        if (!b_ip) {       // 没查到对方的uid ，说明对方不在线
            std::cout << "-----------the user is not in redis,its not online...-----------" << std::endl;
            return;
        }
        //对方在线的情况
        std::cout << "---------- the user is online...-----------" << std::endl;

        auto cfg = ConfigMgr::getInstance();
        auto self_name = (*cfg)["selfServer"]["Name"];
        if (to_ip_value == self_name) {     // 如果是在同一个服务器上，则直接发送，无需跨服通知
            std::cout << "-----the user with self in the serice...-----" << std::endl;
            auto session = UserMgr::getInstance()->GetSession(touid);
            if (session) {
                //在内存中则直接发送通知对方
                Json::Value  notify;
                notify["error"] = ErrorCodes::Success;
                
                std::string base_key = USER_BASE_INFO + std::to_string(fromuid);    // 查找自己的信息并发送给对方
                auto user_info = std::make_shared<UserInfo>();
                bool b_info = GetBaseInfo(base_key, fromuid, user_info);
                if (b_info) {
                    notify["uid"] = user_info->uid;
                    notify["name"] = user_info->name;
                    notify["email"] = user_info->email;
                    notify["nick"] = user_info->nick;
                    notify["desc"] = user_info->desc;
                    notify["sex"] = user_info->sex;
                    notify["icon"] = user_info->icon;
                }
                else {
                    notify["error"] = ErrorCodes::UidInvalid;
                }

                std::string return_str = notify.toStyledString();
                session->Send(return_str, ID_NOTIFY_AUTH_FRIEND_REQ);
            }
            return;
        }

        // 不在自己的服务器上， 通过 grpc 远程调用通知
        AuthFriendReq auth_req;
        auth_req.set_fromuid(fromuid);
        auth_req.set_touid(touid);
        ChatGrpcClient::getInstance()->NotifyAuthFriend(to_ip_value, auth_req);
        };

    // 发送消息
    _fun_call_back[ID_TEXT_CHAT_MSG_REQ] = [this](std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data) {
        Json::Reader reader;
        Json::Value root;
        reader.parse(msg_data, root);

        auto senderuid = root["selfuid"].asInt();
        auto receiveruid = root["touid"].asInt();
        auto message = root["message"].asString();
        auto marking = root["marking"].asString();
        auto type = root["type"].asInt();

        Json::Value rtvalue;
        rtvalue["error"] = ErrorCodes::Success;
        rtvalue["selfuid"] = senderuid;
        rtvalue["touid"] = receiveruid;
        rtvalue["marking"] = marking;
        rtvalue["message"] = message;
        rtvalue["type"] = type;

        Defer defer([this,&rtvalue,session]() {
            std::string return_str = rtvalue.toStyledString();
            session->Send(return_str, ID_TEXT_CHAT_MSG_RSP);
            });

        /// 通过 redis 查找对方服务器
        auto to_str = std::to_string(receiveruid);
        auto to_ip_key = USERIPPREFIX + to_str;
        std::string to_ip_value = "";
        bool b_ip = RedisMgr::getInstance()->Get(to_ip_key, to_ip_value);  // 判断对方是否离线

        // 吧该条数据插入数据库库中
        bool isfinish = MysqlMgr::getInstance()->addMessageNode(senderuid,receiveruid,message,marking,b_ip,type);
        if (!isfinish) {
            rtvalue["error"] = ErrorCodes::SqlInserMessFailed;
            return;
        }
        
        if (!b_ip) {
            std::cout << "对方已下线，数据保存在数据库中..." << std::endl;
            return;            // 如果对方离线则直接返回了，若是在线则发送
        }

        auto cfg = ConfigMgr::getInstance();
        auto self_name = (*cfg)["selfServer"]["Name"];
        if (to_ip_value == self_name) {                       // 如果是在同一个服务器上则直接发送
            std::cout << "对方在本服务器上..." << std::endl;
            auto session = UserMgr::getInstance()->GetSession(receiveruid);
            if (session) {
                std::string return_str = rtvalue.toStyledString();
                session->Send(return_str, ID_NOTIFY_TEXT_CHAT_MSG_REQ);
            }
            return;
        }
        // 不在同一个服务器上，GRPC 远程调用
        std::cout << "对方不在本服务器上..." << std::endl;
        TextChatMsgReq text_msg_req;
        text_msg_req.set_fromuid(senderuid);
        text_msg_req.set_touid(receiveruid);
        text_msg_req.set_type(type);

        auto content = message;
        auto msgid = marking;
        auto* text_msg = text_msg_req.add_textmsgs();
        text_msg->set_msgid(msgid);
        text_msg->set_msgcontent(content);

        ChatGrpcClient::getInstance()->NotifyTextChatMsg(to_ip_value, text_msg_req, rtvalue);

        };

    // 通过uid 获取好友列表信息
    _fun_call_back[ID_GET_FRIEND_LIST_FROM_UID_REQ] = [this](std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data) {
        Json::Reader reader;
        Json::Value root;
        reader.parse(msg_data, root);

        int uid = root["selfuid"].asInt();

        Json::Value rtvalue;
        rtvalue["error"] = ErrorCodes::Success;
        Defer defer([this, &rtvalue, session]() {
            std::string return_str = rtvalue.toStyledString();
            session->Send(return_str, ID_GET_FRIEND_LIST_FROM_UID_RSP);
            });

        // 通过数据库查找返回该用户的好友信息
        bool isfinish = MysqlMgr::getInstance()->getFriendListFromUid(uid,rtvalue);
        if (!isfinish) {
            rtvalue["error"] = ErrorCodes::ID_GET_FRIEND_LIST_FROM_UID_FAILED;
            std::cout << "search friend failed..." << std::endl;
            return;
        }

        };

    // 通过uid 获取好友申请列表信息
    _fun_call_back[ID_GET_FRIEND_REQUESTLIST_FROM_UID_REQ] = [this](std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data) {
        Json::Reader reader;
        Json::Value root;
        reader.parse(msg_data, root);

        int uid = root["selfuid"].asInt();

        Json::Value rtvalue;
        rtvalue["error"] = ErrorCodes::Success;
        Defer defer([this, &rtvalue, session]() {
            std::string return_str = rtvalue.toStyledString();
            session->Send(return_str, ID_GET_FRIEND_REQUESTLIST_FROM_UID_RSP);
            });

        // 通过数据库查找返回该用户的好友信息
        bool isfinish = MysqlMgr::getInstance()->getFriendRequestListFromUid(uid, rtvalue);
        if (!isfinish) {
            rtvalue["error"] = ErrorCodes::ID_GET_FRIEND_REQUESTLIST_FROM_UID_FAILED;
            std::cout << "search friend failed..." << std::endl;
            return;
        }

        };

    // 通过uid 获取聊天记录
    _fun_call_back[ID_GET_CHAT_LIST_FROM_UID_REQ] = [this](std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data) {
        Json::Reader reader;
        Json::Value root;
        reader.parse(msg_data, root);

        int selfuid = root["selfuid"].asInt();
        int useruid = root["useruid"].asInt();
        int index = root["index"].asInt();

        Json::Value rtvalue;
        rtvalue["error"] = ErrorCodes::Success;
        rtvalue["selfuid"] = selfuid;
        rtvalue["useruid"] = useruid;
        rtvalue["index"] = index;
        Defer defer([this, &rtvalue, session]() {
            std::string return_str = rtvalue.toStyledString();
            session->Send(return_str, ID_GET_CHAT_LIST_FROM_UID_RSP);
            });

        // 通过数据库查找返回该用户的好友信息
        bool isfinish = MysqlMgr::getInstance()->getMessageListFromUid(selfuid,useruid, rtvalue);
        if (!isfinish) {
            rtvalue["error"] = ErrorCodes::ID_GET_CHAT_MESSAGE_LIST_FAILED;
            std::cout << "search Message List failed..." << std::endl;
            return;
        }

        };

    // 通过uid 获取更多聊天记录
    _fun_call_back[ID_LOADING_MESSAGE_FROM_UID_REQ] = [this](std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data) {
        Json::Reader reader;
        Json::Value root;
        reader.parse(msg_data, root);

        int selfuid = root["selfuid"].asInt();
        int useruid = root["touid"].asInt();
        std::string messMarking = root["messageMarking"].asString();
        std::string loadMarking = root["loadingMarking"].asString();

        Json::Value rtvalue;
        rtvalue["error"] = ErrorCodes::Success;
        rtvalue["selfuid"] = selfuid;
        rtvalue["useruid"] = useruid;
        rtvalue["messMarking"] = messMarking;
        rtvalue["loadMarking"] = loadMarking;
        rtvalue["message"] = Json::arrayValue;

        Defer defer([this, &rtvalue, session]() {
            std::string return_str = rtvalue.toStyledString();
            session->Send(return_str, ID_LOADING_MESSAGE_FROM_UID_RSP);
            });

        // 通过数据库查找返回该用户的好友信息
        bool isfinish = MysqlMgr::getInstance()->getMoreMessageListFromUid(selfuid, useruid, messMarking ,rtvalue);
        if (!isfinish) {
            rtvalue["error"] = ErrorCodes::ID_GET_CHAT_MESSAGE_LIST_FAILED;
            std::cout << "search MoreMessage List failed..." << std::endl;
            return;
        }

        };

    // 获取 fileserver 接口路径
    _fun_call_back[ID_GET_FILESERVER_PATH_REQ] = [this](std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data) {
        Json::Reader reader;
        Json::Value root;
        reader.parse(msg_data, root);

        Json::Value rtvalue;
        rtvalue["error"] = ErrorCodes::Success;
        rtvalue["is_send"] = root["is_send"].asInt();
       
        Defer defer([this, &rtvalue, session]() {
            std::string return_str = rtvalue.toStyledString();
            session->Send(return_str, ID_GET_FILESERVER_PATH_RSP);
            });

        auto cfg = ConfigMgr::getInstance();
        rtvalue["fileHost"] = (*cfg)["FileServer"]["host"];
        rtvalue["filePort"] = (*cfg)["FileServer"]["Port"];
        
        };


}


void LogicSystem::stop() {
	std::lock_guard<std::mutex> lock(_mutex);
	_stop = true;
	_cond.notify_all();
	if (_work_thread.joinable()) _work_thread.join();
}


//外部调用的接口:将数据放入消息队列，如果处理线程正在等待，则唤醒处理线程
void LogicSystem::PostMsgToQue(std::shared_ptr<LogicNode> logicnode) {
	std::lock_guard<std::mutex> lock(_mutex);
	_msg_que.push(logicnode);
	if (_msg_que.size() == 1) {
		_cond.notify_one();
	}
}


bool LogicSystem::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo) {
    // 先查询redis 中的用户信息
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
        std::cout << "user login uid is : " << userinfo->uid << std::endl;
        std::cout << "user login pwd is : " << userinfo->pwd << std::endl;
        std::cout << "user login email is : " << userinfo->email << std::endl;
    }
    else {
        // redis 没查到就去查询mysql
        std::shared_ptr<UserInfo> user_info = nullptr;
        user_info = MysqlMgr::getInstance()->GetUser(uid);
        if (user_info == nullptr) return false;
        userinfo = user_info;
    }
    //将数据库中的内容写入到redis 缓存
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

    return true;
    
}

void LogicSystem::SetServer(std::shared_ptr<CServer> pserver)
{
    _p_server = pserver;
}

bool LogicSystem::isPureDigit(const std::string& str) {
    for (auto& i : str) {
        if (!std::isdigit(i)) return false;
    }return true;
}

void LogicSystem::GetUserByUid(const std::string& uid_str, Json::Value& rtvalue) {
    std::cout<<"inter the uidSearch function..."<<std::endl;

    rtvalue["error"] = ErrorCodes::Success;
    std::string base_key = USER_BASE_INFO + uid_str;

    // 先从 redis 中查找 uid
    std::string info_str = "";
    bool b_base = RedisMgr::getInstance()->Get(base_key, info_str);
    if (b_base) {
        Json::Reader reader;
        Json::Value root;
        reader.parse(info_str, root);
        auto uid = root["uid"].asInt();
        auto name = root["name"].asString();
        auto pwd = root["pwd"].asString();
        auto email = root["email"].asString();
        auto nick = root["nick"].asString();
        auto desc = root["desc"].asString();
        auto sex = root["sex"].asInt();
        auto icon = root["icon"].asString();
        std::cout << "user  uid is  " << uid << " name  is "
            << name << " pwd is " << pwd << " email is " << email << " icon is " << icon << std::endl;

        rtvalue["uid"] = uid;
        rtvalue["pwd"] = pwd;
        rtvalue["name"] = name;
        rtvalue["email"] = email;
        rtvalue["nick"] = nick;
        rtvalue["desc"] = desc;
        rtvalue["sex"] = sex;
        rtvalue["icon"] = icon;
        return;
    }

    // 如果 reids 中没有数据则从数据库中查找并gengxin redis
    auto uid = std::stoi(uid_str);
    std::shared_ptr<UserInfo> user_info = nullptr;
    user_info = MysqlMgr::getInstance()->GetUser(uid);
    if (user_info == nullptr) {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    Json::Value redis_root;
    redis_root["uid"] = user_info->uid;
    redis_root["pwd"] = user_info->pwd;
    redis_root["name"] = user_info->name;
    redis_root["email"] = user_info->email;
    redis_root["nick"] = user_info->nick;
    redis_root["desc"] = user_info->desc;
    redis_root["sex"] = user_info->sex;
    redis_root["icon"] = user_info->icon;

    // 将数据插入至 redis
    RedisMgr::getInstance()->Set(base_key, redis_root.toStyledString());
    auto server_name = ConfigMgr::getInstance()->GetValue("SelfServer", "Name");

    auto rd_res = RedisMgr::getInstance()->HGet(LOGIN_COUNT, server_name);
    int count = 0;
    if (!rd_res.empty()) {
        count = std::stoi(rd_res);
    }

    count++;
    auto count_str = std::to_string(count);
    RedisMgr::getInstance()->HSet(LOGIN_COUNT, server_name, count_str);

    /// 设置返回值
    rtvalue["uid"] = user_info->uid;
    rtvalue["pwd"] = user_info->pwd;
    rtvalue["name"] = user_info->name;
    rtvalue["email"] = user_info->email;
    rtvalue["nick"] = user_info->nick;
    rtvalue["desc"] = user_info->desc;
    rtvalue["sex"] = user_info->sex;
    rtvalue["icon"] = user_info->icon;
}
void LogicSystem::GetUserByName(const std::string& name, Json::Value& rtvalue) {
    std::cout << "inter the nameSearch function..." << std::endl;

    rtvalue["error"] = ErrorCodes::Success;
    std::string base_key = NAME_INFO + name;

    // 先从 redis 中查找这个用户名
    std::string info_str = "";
    bool b_base = RedisMgr::getInstance()->Get(base_key, info_str);
    if (b_base) {
        Json::Reader reader;
        Json::Value root;
        reader.parse(info_str, root);
        auto uid = root["uid"].asInt();
        auto name = root["name"].asString();
        auto pwd = root["pwd"].asString();
        auto email = root["email"].asString();
        auto nick = root["nick"].asString();
        auto desc = root["desc"].asString();
        auto sex = root["sex"].asInt();
        std::cout << "user  uid is  " << uid << " name  is "
            << name << " pwd is " << pwd << " email is " << email << std::endl;

        rtvalue["uid"] = uid;
        rtvalue["pwd"] = pwd;
        rtvalue["name"] = name;
        rtvalue["email"] = email;
        rtvalue["nick"] = nick;
        rtvalue["desc"] = desc;
        rtvalue["sex"] = sex;
        return;
    }

    // redis 中没有这个用户名则查找数据库
    std::shared_ptr<UserInfo> user_info = nullptr;
    user_info = MysqlMgr::getInstance()->GetUser(name);
    if (user_info == nullptr) {
        rtvalue["error"] = ErrorCodes::UidInvalid;             // 数据库 和 redis 中都没有该用户的信息则返回 ErrorCodes::UidInvalid
        return;
    }
    Json::Value redis_root;
    // 写入到redis 缓存中
    redis_root["uid"] = user_info->uid;
    redis_root["pwd"] = user_info->pwd;
    redis_root["name"] = user_info->name;
    redis_root["email"] = user_info->email;
    redis_root["nick"] = user_info->nick;
    redis_root["desc"] = user_info->desc;
    redis_root["sex"] = user_info->sex;

    RedisMgr::getInstance()->Set(base_key, redis_root.toStyledString());

    rtvalue["uid"] = user_info->uid;
    rtvalue["pwd"] = user_info->pwd;
    rtvalue["name"] = user_info->name;
    rtvalue["email"] = user_info->email;
    rtvalue["nick"] = user_info->nick;
    rtvalue["desc"] = user_info->desc;
    rtvalue["sex"] = user_info->sex;
}




















