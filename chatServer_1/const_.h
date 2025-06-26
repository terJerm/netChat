#pragma once
#include <functional>
#include <string>

#define MAX_LENGTH 1024 * 2

#define HEAD_TOTAL_LEN 4
#define HEAD_ID_LEN 2
#define HEAD_DATA_LEN 2

#define MAX_RECVQUE 10000         //最大接受数据的字节数
#define MAX_SENDQUE 1000          //最大发送队列数


enum MSG_IDS {
	MSG_CHAT_LOGIN = 1005, //用户登陆
	MSG_CHAT_LOGIN_RSP = 1006, //用户登陆回包
	ID_SEARCH_USER_REQ = 1007, //用户搜索请求
	ID_SEARCH_USER_RSP = 1008, //搜索用户回包
	ID_ADD_FRIEND_REQ = 1009, //申请添加好友请求
	ID_ADD_FRIEND_RSP = 1010, //申请添加好友回复
	ID_NOTIFY_ADD_FRIEND_REQ = 1011,  //通知用户添加好友申请
	ID_AUTH_FRIEND_REQ = 1013,  //认证好友请求
	ID_AUTH_FRIEND_RSP = 1014,  //认证好友回复
	ID_NOTIFY_AUTH_FRIEND_REQ = 1015, //通知用户认证好友申请
	ID_TEXT_CHAT_MSG_REQ = 1017, //文本聊天信息请求
	ID_TEXT_CHAT_MSG_RSP = 1018, //文本聊天信息回复
	ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019, //通知用户文本聊天信息

	ID_GET_FRIEND_LIST_FROM_UID_REQ = 1020,
	ID_GET_FRIEND_LIST_FROM_UID_RSP = 1021,

	ID_GET_FRIEND_REQUESTLIST_FROM_UID_REQ = 1022,
	ID_GET_FRIEND_REQUESTLIST_FROM_UID_RSP = 1023,

	ID_GET_CHAT_LIST_FROM_UID_REQ = 1024,
	ID_GET_CHAT_LIST_FROM_UID_RSP = 1025,

	ID_LOADING_MESSAGE_FROM_UID_REQ = 1026,             /// 加载更多聊天记录
	ID_LOADING_MESSAGE_FROM_UID_RSP = 1027,

	ID_GET_FILESERVER_PATH_REQ = 1028,                  /// 获取 fileserver 接口地址
	ID_GET_FILESERVER_PATH_RSP = 1029,

	ID_NOTIFY_OFF_LINE_REQ = 1030,                      /// 异地登陆，通知下线

	ID_HEART_BEAT_MESSAGE_REQ = 1050,
	ID_HEART_BEAT_MESSAGE_RSP = 1051,

};

enum ErrorCodes {
	Success = 0,
	Error_Json = 1001,            //Json解析错误
	RPCFailed = 1002,             //RPC请求错误
	VarifyExpired = 1003,         //验证码过期
	VarifyCodeErr = 1004,         //验证码错误
	UserExist = 1005,             //用户已经存在
	PasswdErr = 1006,             //密码错误
	EmailNotMatch = 1007,         //邮箱不匹配
	PasswdUpFailed = 1008,        //更新密码失败
	PasswdInvalid = 1009,         //密码更新失败
	TokenInvalid = 1010,          //Token失效
	UidInvalid = 1011,            //uid无效
	SqlInserMessFailed = 1012,    //聊天信息插入数据库失败

	ID_GET_FRIEND_LIST_FROM_UID_FAILED = 1013,
	ID_GET_FRIEND_REQUESTLIST_FROM_UID_FAILED = 1014,
	ID_GET_CHAT_MESSAGE_LIST_FAILED = 1015,
};

class UserInfo {
public:
	UserInfo();
	std::string name;
	std::string pwd;
	int uid;
	std::string email;
	std::string nick;
	std::string desc;
	int sex;
	std::string icon;
	std::string back;
};
struct ApplyInfo {
	ApplyInfo(int uid, std::string name, std::string desc, std::string icon, std::string nick, int sex, int status);
	int _uid;
	std::string _name;
	std::string _desc;
	std::string _icon;
	std::string _nick;
	int _sex;
	int _status;
};


class Defer {
private:
	std::function<void()> _func;
public:
	Defer(std::function<void()> func);
	~Defer();
};


std::string trim(const std::string& str);


#define USERIPPREFIX  "uip_"
#define USERTOKENPREFIX  "utoken_"
#define IPCOUNTPREFIX  "ipcount_"
#define USER_BASE_INFO "ubaseinfo_"
#define LOGIN_COUNT  "logincount"
#define NAME_INFO  "nameinfo_"

#define LOCK_PREFIX "lock_"
#define USER_SESSION_PREFIX "usession_"
#define LOCK_COUNT "lockcount"
#define LOCK_TIME_OUT 10
#define ACQUIRE_TIME_OUT 5



