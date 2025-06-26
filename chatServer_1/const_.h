#pragma once
#include <functional>
#include <string>

#define MAX_LENGTH 1024 * 2

#define HEAD_TOTAL_LEN 4
#define HEAD_ID_LEN 2
#define HEAD_DATA_LEN 2

#define MAX_RECVQUE 10000         //���������ݵ��ֽ���
#define MAX_SENDQUE 1000          //����Ͷ�����


enum MSG_IDS {
	MSG_CHAT_LOGIN = 1005, //�û���½
	MSG_CHAT_LOGIN_RSP = 1006, //�û���½�ذ�
	ID_SEARCH_USER_REQ = 1007, //�û���������
	ID_SEARCH_USER_RSP = 1008, //�����û��ذ�
	ID_ADD_FRIEND_REQ = 1009, //������Ӻ�������
	ID_ADD_FRIEND_RSP = 1010, //������Ӻ��ѻظ�
	ID_NOTIFY_ADD_FRIEND_REQ = 1011,  //֪ͨ�û���Ӻ�������
	ID_AUTH_FRIEND_REQ = 1013,  //��֤��������
	ID_AUTH_FRIEND_RSP = 1014,  //��֤���ѻظ�
	ID_NOTIFY_AUTH_FRIEND_REQ = 1015, //֪ͨ�û���֤��������
	ID_TEXT_CHAT_MSG_REQ = 1017, //�ı�������Ϣ����
	ID_TEXT_CHAT_MSG_RSP = 1018, //�ı�������Ϣ�ظ�
	ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019, //֪ͨ�û��ı�������Ϣ

	ID_GET_FRIEND_LIST_FROM_UID_REQ = 1020,
	ID_GET_FRIEND_LIST_FROM_UID_RSP = 1021,

	ID_GET_FRIEND_REQUESTLIST_FROM_UID_REQ = 1022,
	ID_GET_FRIEND_REQUESTLIST_FROM_UID_RSP = 1023,

	ID_GET_CHAT_LIST_FROM_UID_REQ = 1024,
	ID_GET_CHAT_LIST_FROM_UID_RSP = 1025,

	ID_LOADING_MESSAGE_FROM_UID_REQ = 1026,             /// ���ظ��������¼
	ID_LOADING_MESSAGE_FROM_UID_RSP = 1027,

	ID_GET_FILESERVER_PATH_REQ = 1028,                  /// ��ȡ fileserver �ӿڵ�ַ
	ID_GET_FILESERVER_PATH_RSP = 1029,

	ID_NOTIFY_OFF_LINE_REQ = 1030,                      /// ��ص�½��֪ͨ����

	ID_HEART_BEAT_MESSAGE_REQ = 1050,
	ID_HEART_BEAT_MESSAGE_RSP = 1051,

};

enum ErrorCodes {
	Success = 0,
	Error_Json = 1001,            //Json��������
	RPCFailed = 1002,             //RPC�������
	VarifyExpired = 1003,         //��֤�����
	VarifyCodeErr = 1004,         //��֤�����
	UserExist = 1005,             //�û��Ѿ�����
	PasswdErr = 1006,             //�������
	EmailNotMatch = 1007,         //���䲻ƥ��
	PasswdUpFailed = 1008,        //��������ʧ��
	PasswdInvalid = 1009,         //�������ʧ��
	TokenInvalid = 1010,          //TokenʧЧ
	UidInvalid = 1011,            //uid��Ч
	SqlInserMessFailed = 1012,    //������Ϣ�������ݿ�ʧ��

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



