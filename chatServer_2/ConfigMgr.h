#pragma once
#include "SingletonPtr.h"
#include <fstream>  
#include <boost/property_tree/ptree.hpp>  
#include <boost/property_tree/ini_parser.hpp>  
#include <boost/filesystem.hpp>    
#include <map>
#include <iostream>

struct SectionInfo {
	using string = std::string;
	std::map<string, string> _section_datas;

	SectionInfo();
	~SectionInfo();
	SectionInfo(const SectionInfo& src);
	SectionInfo& operator = (const SectionInfo& src);
	std::string  operator[](const std::string& key);
	std::string GetValue(const std::string& key);
};



class ConfigMgr:public SingletonPtr<ConfigMgr>{
	friend class SingletonPtr<ConfigMgr>;
	using string = std::string;
private:
	std::map<string, SectionInfo> _config_map;
private:
	ConfigMgr();
public:
	~ConfigMgr();
	ConfigMgr(const ConfigMgr&) = delete;
	ConfigMgr& operator = (const ConfigMgr&) = delete;
	std::string GetValue(const std::string& section, const std::string& key);
	SectionInfo operator[](const std::string& section);

};

