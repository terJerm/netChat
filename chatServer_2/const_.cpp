#include "const_.h"
#include <algorithm>


Defer::Defer(std::function<void()> func) :_func(func){

}

Defer::~Defer() {
	_func();
}

std::string trim(const std::string& str) {
    auto front = std::find_if_not(str.begin(), str.end(), ::isspace);
    auto back = std::find_if_not(str.rbegin(), str.rend(), ::isspace).base();
    return (front < back ? std::string(front, back) : std::string());
}

UserInfo::UserInfo():name(""),pwd(""),uid(0),email(""),nick(""),desc(""),sex(0),icon(""),back("") {}

ApplyInfo::ApplyInfo(int uid, std::string name, std::string desc, std::string icon, std::string nick, int sex, int status)
    :_uid(uid),_name(name),_desc(desc),_icon(icon),_nick(nick),_sex(sex),_status(status){}










