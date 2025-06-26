#include "HttpConnection.h"
#include "LogicSystem.h"

unsigned char ToHex(unsigned char x){
	return  x > 9 ? x + 55 : x + 48;
}

unsigned char FromHex(unsigned char x)
{
	unsigned char y;
	if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
	else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
	else if (x >= '0' && x <= '9') y = x - '0';
	else assert(0);
	return y;
}

std::string UrlEncode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//�ж��Ƿ�������ֺ���ĸ����
		if (isalnum((unsigned char)str[i]) ||
			(str[i] == '-') ||
			(str[i] == '_') ||
			(str[i] == '.') ||
			(str[i] == '~'))
			strTemp += str[i];
		else if (str[i] == ' ') //Ϊ���ַ�
			strTemp += "+";
		else
		{
			//�����ַ���Ҫ��ǰ��%���Ҹ���λ�͵���λ�ֱ�תΪ16����
			strTemp += '%';
			strTemp += ToHex((unsigned char)str[i] >> 4);
			strTemp += ToHex((unsigned char)str[i] & 0x0F);
		}
	}
	return strTemp;
}

std::string UrlDecode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//��ԭ+Ϊ��
		if (str[i] == '+') strTemp += ' ';
		//����%������������ַ���16����תΪchar��ƴ��
		else if (str[i] == '%')
		{
			assert(i + 2 < length);
			unsigned char high = FromHex((unsigned char)str[++i]);
			unsigned char low = FromHex((unsigned char)str[++i]);
			strTemp += high * 16 + low;
		}
		else strTemp += str[i];
	}
	return strTemp;
}

HttpConnection::HttpConnection(boost::asio::io_context& ioc) :_socket(ioc){

}

void HttpConnection::start() {
	auto self = shared_from_this();
	//���ͻ��˷������� http get/post ���� 
	http::async_read(_socket, _buffer, _request, [self](beast::error_code ec,std::size_t bytes) {
		try {
			if (ec) {
				std::cout << "http read err is: " << ec.what() << std::endl;
				return;
			}
			boost::ignore_unused(bytes);
			self->HandleReq();      //��������
			self->CheckDeadline();  //��ʱ���        1. �������� 2. �ظ����� 3. ��ʱ��� 
		}
		catch(std::exception& ec){
			std::cout << "exception is: " << ec.what() << std::endl;
		}
		});
}

void HttpConnection::CheckDeadline() {
	auto self = shared_from_this();
	_deadline.async_wait([self](beast::error_code ec) {        //������ʱ�����첽�ȴ�
		if (!ec) {       //û�д����첽�ȵ��ˣ�˵����ʱ
			self->_socket.close();
		}
		});
}
void HttpConnection::WriteResponce() {
	auto self = shared_from_this();
	_response.content_length(_response.body().size());
	//�ذ������ͻ��˷�������
	http::async_write(
		_socket,
		_response,
		[self](beast::error_code ec, std::size_t){
			//������û�з��ͳɹ����Ͽ����Ͷˣ����ˣ����ӣ��رձ��ζ�ʱ��
			self->_socket.shutdown(tcp::socket::shutdown_send, ec);
			self->_deadline.cancel();
		});
}
void HttpConnection::HandleReq() {
	_response.version(_request.version());      //���ð汾
	_response.keep_alive(false);                //����Ϊ������

	if (_request.method() == http::verb::get) { //����get����
		PreParseGetParam();
		bool success = LogicSystem::getInstance().handleGet(_get_url, shared_from_this());//·�ɡ�����
		if (!success) {                         //
			_response.result(http::status::not_found);
			_response.set(http::field::content_type, "text/plain");
			///�ظ�����: �� body() ��д������
			beast::ostream(_response.body()) << "url not found\r\n";
			WriteResponce();
			return;
		}                                       //
		_response.result(http::status::ok);
		_response.set(http::field::server, "GateServer");
		WriteResponce();
		return;
	}else if (_request.method() == http::verb::post) { //����post����
		PreParseGetParam();
		bool success = LogicSystem::getInstance().handlePost(_request.target(), shared_from_this());//·�ɡ�����
		if (!success) {                         //
			_response.result(http::status::not_found);
			_response.set(http::field::content_type, "text/plain");
			beast::ostream(_response.body()) << "url not found\r\n";
			WriteResponce();
			return;
		}                                       //
		_response.result(http::status::ok);
		_response.set(http::field::server, "GateServer");
		WriteResponce();
		return;
	}
}


//get ����Ĳ�������
void HttpConnection::PreParseGetParam() {
	// ��ȡ URI  ��/get_test��key1 = value1 & key2 = value2
	auto uri = _request.target();  
	auto query_pos = uri.find('?');
	if (query_pos == std::string::npos) {                 //û�ҵ��������������󲻴��������������� url
		_get_url = uri;
		return;
	}
	_get_url = uri.substr(0, query_pos);                  //ǰ�� url ����
	std::string query_string = uri.substr(query_pos + 1); //������� Key:Value ����
	std::string key;
	std::string value;
	size_t pos = 0;
	while ((pos = query_string.find('&')) != std::string::npos) {
		auto pair = query_string.substr(0, pos);
		size_t eq_pos = pair.find('=');
		if (eq_pos != std::string::npos) {
			key = UrlDecode(pair.substr(0, eq_pos)); // ������ url_decode ����������URL����  
			value = UrlDecode(pair.substr(eq_pos + 1));
			_get_params[key] = value;
		}
		query_string.erase(0, pos + 1);
	}
	// �������һ�������ԣ����û�� & �ָ�����  
	if (!query_string.empty()) {
		size_t eq_pos = query_string.find('=');
		if (eq_pos != std::string::npos) {
			key = UrlDecode(query_string.substr(0, eq_pos));
			value = UrlDecode(query_string.substr(eq_pos + 1));
			_get_params[key] = value;
		}
	}
}

tcp::socket& HttpConnection::getSocket() {
	return _socket;
}
