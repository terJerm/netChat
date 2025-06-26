#include "usermgr.h"

UserMgr::UserMgr(){

}

UserMgr::~UserMgr()
{

}

void UserMgr::setName(QString name)
{
    _name = name;
}

void UserMgr::setToken(QString token)
{
    _token = token;
}

void UserMgr::SetUid(int uid)
{
    _uid = uid;
}

void UserMgr::setSex(int sex)
{
    _sex = sex;
}

void UserMgr::setIcon(QString icon)
{
    _icon = icon;
}

void UserMgr::setEmail(QString email)
{
    _email = email;
}

void UserMgr::setDesc(QString desc)
{
    _desc = desc;
}

QString UserMgr::getName()
{
    return _name;
}

QString UserMgr::getToken()
{
    return _token;
}

int UserMgr::getUid()
{
    return _uid;
}

int UserMgr::getSex()
{
    return _sex;
}

QString UserMgr::getIcon()
{
    return _icon;
}

QString UserMgr::getEmail()
{
    return _email;
}

QString UserMgr::getDesc()
{
    return _desc;
}


