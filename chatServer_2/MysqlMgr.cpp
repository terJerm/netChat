#include "MysqlMgr.h"

MysqlMgr::MysqlMgr() {

}


MysqlMgr::~MysqlMgr() {

}
int MysqlMgr::regUser(const std::string& name, const std::string& email, const std::string& pwd) {
	return _dao.regUser(name, email, pwd);
}

bool MysqlMgr::CheckEmail(const std::string& name, const std::string& email) {
	return _dao.CheckEmail(name, email);
}
bool MysqlMgr::UpdatePwd(const std::string& name, const std::string& pwd) {
	return _dao.UpdatePwd(name, pwd);
}

bool MysqlMgr::CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo) {
	return _dao.CheckPwd(name, pwd, userInfo);
}

bool MysqlMgr::addFriendApply(const int& uid1, const int& uid2, const std::string& mess, const std::string& nick) {
	return _dao.addFriendApply(uid1, uid2, mess, nick);
}

std::shared_ptr<UserInfo> MysqlMgr::GetUser(int uid){
	return _dao.GetUser(uid);
}

std::shared_ptr<UserInfo> MysqlMgr::GetUser(std::string uid){
	return _dao.GetUser(uid);
}

bool MysqlMgr::AuthFriendApply(int selfuid, int touid ) {
	return _dao.AuthFriendApply(selfuid,touid);
}
bool MysqlMgr::AddFriend(int selfuid, int touid) {
	return _dao.AddFriend(selfuid, touid);
}

bool MysqlMgr::addMessageNode(const int& senderuid, const int& receiveruid, std::string& message, std::string& marking, bool& b_ip,int type) {
	return _dao.addMessageNode(senderuid, receiveruid, message, marking, b_ip,type);
}

bool MysqlMgr::getFriendListFromUid( int& uid,  Json::Value& rtvalue)
{
	return _dao.getFriendListFromUid(uid,rtvalue);
}

bool MysqlMgr::getFriendRequestListFromUid(int& uid, Json::Value& rtvalue)
{
	return _dao.getFriendRequestListFromUid(uid,rtvalue);
}

bool MysqlMgr::getMessageListFromUid(int& selfuid, int& useruid, Json::Value& rtvalue)
{
	return _dao.getMessageListFromUid(selfuid,useruid,rtvalue);
}

bool MysqlMgr::getMoreMessageListFromUid(int& selfuid, int& useruid, std::string& marking, Json::Value& rtvalue)
{
	return _dao.getMoreMessageListFromUid(selfuid, useruid, marking, rtvalue);
}






