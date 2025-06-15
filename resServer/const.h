#pragma once
#include <functional>
#include <string>


enum ErrorCodes {
	Success = 0,
	Error_Json = 1001,  
	RPCFailed = 1002,  
	VarifyExpired = 1003, 
	VarifyCodeErr = 1004, 
	UserExist = 1005,       
	PasswdErr = 1006,    
	EmailNotMatch = 1007,  
	PasswdUpFailed = 1008, 
	PasswdInvalid = 1009,   
	TokenInvalid = 1010,   
	UidInvalid = 1011,  
};


std::string gbkToUtf8(const std::string& gbkStr);


class Defer {
public:
	
	Defer(std::function<void()> func) : func_(func) {}

	
	~Defer() {
		func_();
	}

private:
	std::function<void()> func_;
};

#define MAX_LENGTH  2048 * 10

#define HEAD_TOTAL_LEN 6

#define HEAD_ID_LEN 2

#define HEAD_DATA_LEN 4
#define MAX_RECVQUE  2000000
#define MAX_SENDQUE 2000000


#define LOGIC_WORKER_COUNT 4

#define FILE_WORKER_COUNT 4


enum MSG_IDS {

	ID_UPLOAD_FILE_REQ = 10001,                         
	ID_UPLOAD_FILE_RSP = 10002,

	iD_GET_FILE_FROM_MARKING_REQ = 10003,
	iD_GET_FILE_FROM_MARKING_RSP = 10004,
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

#define USERIPPREFIX  "uip_"
#define USERTOKENPREFIX  "utoken_"
#define IPCOUNTPREFIX  "ipcount_"
#define USER_BASE_INFO "ubaseinfo_"
#define LOGIN_COUNT  "logincount"
#define NAME_INFO  "nameinfo_"


struct fileType {
	std::string filemarking;
	std::string filename;
	int64_t filesize;
	std::string filepath;
};

