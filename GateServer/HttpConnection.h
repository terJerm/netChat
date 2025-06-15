#pragma once

#include "const.h"

//���ӻỰ��
class HttpConnection:public std::enable_shared_from_this<HttpConnection>{
	friend class LogicSystem;
private:
	tcp::socket _socket;                                //���ӵĶ���
	beast::flat_buffer _buffer{ 8192 };                 //������������
	http::request<http::dynamic_body> _request;         //�������������������
	http::response<http::dynamic_body> _response;       //��Ӧ����������Ӧ
	net::steady_timer _deadline{                        //��ʱ���ж��Ƿ�ʱ
		_socket.get_executor(),std::chrono::seconds(10)
	};

	std::string _get_url;                                             //������������ url
	std::unordered_map<std::string, std::string> _get_params;         //�����Ĳ�����key:value

public:
	HttpConnection(boost::asio::io_context& ioc);
	void start();
	tcp::socket& getSocket();
private:
	void CheckDeadline();  //��ʱ���
	void WriteResponce();  //Ӧ��ظ�
	void HandleReq();      //��������
	void PreParseGetParam();               ////get ����Ĳ�������

};

