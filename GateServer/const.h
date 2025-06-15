#pragma once


#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <iostream>
#include <map>
#include <functional>
#include <unordered_map>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <mutex>


namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;


class ConfigMgr;
extern ConfigMgr config_mgr;


enum ErrorCodes {
	SUCCESS = 0,             //�ɹ�
	ERROR_JSON = 1001,       //json ��������
	RPCFAILED = 1002,        //RPC �������
	VarifyExpired = 1003,    //��֤�����
	VarifyCodeErr = 1004,    //��֤�����
	UserExist = 1005,        //�û��Ѿ�����
	PasswdErr = 1006,        //�������
	EmailNotMatch = 1007,    //���䲻ƥ��
	PasswdUpFailed = 1008,   //��������ʧ��
	PasswdInvalid = 1009,    //��������ʧ��
	RPCGetFailed = 1010,
	RPCFailed = 1011,
};

#define CODEPREFIX "code_"

