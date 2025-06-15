#include "MsgNode.h"
#include <string>
#include <iostream>
#include "const_.h"
#include <boost/asio.hpp>

MsgNode::MsgNode(short max_len):_total_len(max_len),_cur_len(0) {
	_data = new char[_total_len + 1];
	_data[_total_len] = '\0';
}

MsgNode::~MsgNode() {
	std::cout << "msgNode ��������..." << std::endl;
	delete[] _data;
}

void MsgNode::clear() {
	::memset(_data, 0, _total_len);
	_cur_len = 0;
}

RecvNode::RecvNode(short max_len, short msg_id):MsgNode(max_len),_msg_id(msg_id) {

}
SendNode::SendNode(const char* msg, short max_len, short msg_id):MsgNode(max_len+HEAD_TOTAL_LEN),
		_msg_id(msg_id){
	//�����head->id, תΪ�����ֽ���
	short msg_id_host = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
	memcpy(_data, &msg_id_host, HEAD_ID_LEN);
	//�����head->len,תΪ�����ֽ���
	short max_len_host = boost::asio::detail::socket_ops::host_to_network_short(max_len);
	memcpy(_data + HEAD_ID_LEN, &max_len_host, HEAD_DATA_LEN);
	//��Ϣ��
	memcpy(_data + HEAD_ID_LEN + HEAD_DATA_LEN, msg, max_len);
}