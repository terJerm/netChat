#pragma once
#include "const.h"

//管理 key 和 value
struct SectionInfo {
	std::map<std::string, std::string> _section_datas;   //存储键值对

	std::string  operator[](const std::string& key);     //重载 [] ,获取 value
	SectionInfo();
	~SectionInfo();
	SectionInfo(const SectionInfo& src);
	SectionInfo& operator = (const SectionInfo& src);
};


//管理和读取 config.ini 配置文件
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

