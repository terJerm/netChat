#pragma once
#include "const.h"

//���� key �� value
struct SectionInfo {
	std::map<std::string, std::string> _section_datas;   //�洢��ֵ��

	std::string  operator[](const std::string& key);     //���� [] ,��ȡ value
	SectionInfo();
	~SectionInfo();
	SectionInfo(const SectionInfo& src);
	SectionInfo& operator = (const SectionInfo& src);
};


//����Ͷ�ȡ config.ini �����ļ�
class ConfigMgr{
private:
	std::map<std::string, SectionInfo> _config_map;
	ConfigMgr();
public:
	~ConfigMgr();
	SectionInfo operator[](const std::string& section);
	ConfigMgr& operator=(const ConfigMgr& src) = delete;
	ConfigMgr(const ConfigMgr& src) = delete;
	static ConfigMgr& getInstance();
};

