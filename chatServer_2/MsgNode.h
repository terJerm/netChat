#pragma once

class LogicSystem;

class MsgNode{
public:
	short _cur_len;
	short _total_len;
	char* _data;
	MsgNode(short max_len);
	virtual ~MsgNode();
	void clear();
};

class RecvNode :public MsgNode {
	friend class LogicSystem;
private:
	short _msg_id;
public:
	RecvNode(short max_len, short msg_id);
};
class SendNode :public MsgNode {
	friend class LogicSystem;
public:
	SendNode(const char* msg, short max_len, short msg_id);
private:
	short _msg_id;
};


