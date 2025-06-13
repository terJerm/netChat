#include "UserMgr.h"
#include "CSession.h"

UserMgr::UserMgr() {
}


UserMgr::~UserMgr() {
	_uid_to_session.clear();
}

std::shared_ptr<CSession> UserMgr::GetSession(int uid) {
	std::lock_guard<std::mutex> lock(_session_mtx);
	auto iter = _uid_to_session.find(uid);
	if (iter == _uid_to_session.end()) {
		return nullptr;
	}
	return iter->second;
}
void UserMgr::SetUserSession(int uid, std::shared_ptr<CSession> session) {
	std::lock_guard<std::mutex> lock(_session_mtx);
	_uid_to_session[uid] = session;
}
void UserMgr::RmvUserSession(int uid) {
	auto uid_str = std::to_string(uid);
	//��Ϊ�ٴε�¼���������������������Ի���ɱ�������ɾ��key������������ע��key�����
	// �п������������¼������ɾ��key����Ҳ���key�����

	//RedisMgr::GetInstance()->Del(USERIPPREFIX + uid_str);

	{
		std::lock_guard<std::mutex> lock(_session_mtx);
		_uid_to_session.erase(uid);
	}
}

void UserMgr::RmvUserSession(int uid, std::string session_id)
{
	{
		std::lock_guard<std::mutex> lock(_session_mtx);
		auto iter = _uid_to_session.find(uid);
		if (iter != _uid_to_session.end()) {
			return;
		}

		auto session_id_ = iter->second->GetSessionId();
		//�����˵���������ط���¼��
		if (session_id_ != session_id) {
			return;
		}
		_uid_to_session.erase(uid);
	}
}


