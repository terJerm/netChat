#pragma once

#include "const.h"
#include "Singleton.h"

class HttpConnection;
typedef std::function<void(std::shared_ptr<HttpConnection>)>httpHandler; //�ص���������

class LogicSystem:public Singleton<LogicSystem>{
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem();
	//ע��һ�� get ����
	void RegGet(std::string, httpHandler handler);    // ·�ɵ�ַ���Ự����
	void RegPost(std::string, httpHandler handler);
	//����һ�� get ����
	bool handleGet(std::string , std::shared_ptr<HttpConnection>);//·�ɵ�ַ���Ự����ptr
	bool handlePost(std::string, std::shared_ptr<HttpConnection>);
private:
	LogicSystem();
	//post�����get����Ļص�����map��keyΪ·�ɣ�valueΪ�ص�������
	std::map<std::string, httpHandler> _post_handlers;
	std::map<std::string, httpHandler> _get_handlers;
};











