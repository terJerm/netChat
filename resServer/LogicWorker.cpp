#include "LogicWorker.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "FileSystem.h"
#include "CSession.h"
#include "ConfigMgr.h"
#include <filesystem>
#include "MysqlMgr.h"
#include <filesystem>
#include "base64.h"

std::wstring stringToWString(const std::string& str) {
	int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
	std::wstring wstr(len, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], len);
	wstr.pop_back();
	return wstr;
}


LogicWorker::LogicWorker():_b_stop(false)
{
	RegisterCallBacks();

	_work_thread = std::thread([this]() {
		while (!_b_stop) {
			std::unique_lock<std::mutex> lock(_mtx);
			_cv.wait(lock, [this]() {
				if(_b_stop) {
					return true;
				}

				if (_task_que.empty()) {
					return false;
				}

				return true;

			});

			if (_b_stop) {
				return;
			}

			auto task = _task_que.front();
			task_callback(task);
			_task_que.pop();
		}
	});

}

LogicWorker::~LogicWorker()
{
	_b_stop = true;
	_cv.notify_one();
	_work_thread.join();
}

void LogicWorker::PostTask(std::shared_ptr<LogicNode> task)
{
	std::lock_guard<std::mutex> lock(_mtx);
	_task_que.push(task);
	_cv.notify_one();
}

void LogicWorker::RegisterCallBacks()
{
	/// �ͻ����ϴ��ļ�
	_fun_callbacks[ID_UPLOAD_FILE_REQ] = [this](shared_ptr<CSession> session, const short& msg_id,
		const string& msg_data) {
			Json::Reader reader;
			Json::Value root;
			reader.parse(msg_data, root);
			auto seq = root["seq"].asInt();                  /// ����Ƭ��
			auto last_seq = root["last_seq"].asInt();        /// ��Ƭ����
			auto name = root["name"].asString();             /// �ļ���
			int64_t total_size = std::stoll(root["total_size"].asString());
			int64_t trans_size = std::stoll(root["trans_size"].asString());
			auto uid = root["uid"].asInt();

			auto last = root["last"].asInt();                /// �Ƿ���      
			auto file_data = root["data"].asString();        /// ����������

			Json::Value  rtvalue;
			Defer defer([this, &rtvalue, session]() {
				std::string return_str = rtvalue.toStyledString();
				session->Send(return_str, ID_UPLOAD_FILE_RSP);
				});

			// ʹ�� std::hash ���ַ������й�ϣ
			std::hash<std::string> hash_fn;
			size_t hash_value = hash_fn(name); // ���ɹ�ϣֵ
			int index = hash_value % FILE_WORKER_COUNT;
			std::cout << "Hash value: " << hash_value << std::endl;

			FileSystem::GetInstance()->PostMsgToQue(
				std::make_shared<FileTask>(session, name, seq, total_size,trans_size, last, file_data),index);

			if (last) {

				// 1. ����Ψһ��ʶ����hash + ʱ�����
				std::string file_id = std::to_string(std::hash<std::string>{}(name + std::to_string(std::time(nullptr))));
				// 2. ��ȡ�����еľ�̬Ŀ¼·��
				std::string rawPath = ConfigMgr::Inst()["RESOURCES"]["path"];

				// ����ĩβ�հ��ַ����ر��� Windows �� \r �� \n
				while (!rawPath.empty() && std::isspace(static_cast<unsigned char>(rawPath.back()))) {
					rawPath.pop_back();
				}
				std::string filepath = rawPath + "/" + name;

				// 4. ����·��ʹ�� .string() ���ּ��ݣ����� char8_t��
				//std::string filepath = fullPath.string();  // ��ʹ�� u8string() ȡ������� MySQL �ַ�������
				std::cout << "filpath is : " << filepath << std::endl;

				// 5. д�����ݿ�
				std::string tip = "none";
				bool success = MysqlMgr::GetInstance()->insertFileInfo(file_id, name, total_size, uid, filepath, tip);

				if (!success) {
					std::cout << "�ļ���Ϣ�������ݿ�ʧ��..." << std::endl;
				}
		
				rtvalue["marking"] = file_id;
			}

			rtvalue["error"] = ErrorCodes::Success;
			rtvalue["total_size"] = std::to_string(total_size);
			rtvalue["trans_size"] = std::to_string(trans_size);
			rtvalue["seq"] = seq;
			rtvalue["name"] = name;
			rtvalue["last"] = last;
		};



	/// �ͻ��������ļ�
	_fun_callbacks[iD_GET_FILE_FROM_MARKING_REQ] = [this](shared_ptr<CSession> session, const short& msg_id,const string& msg_data) {

		std::cout << "receive the iD_GET_FILE_FROM_MARKING_REQ" << std::endl;
		Json::Reader reader;
			Json::Value root;
			reader.parse(msg_data, root);
			auto marking = root["marking"].asString();

			Json::Value  rtvalue;	
			// �����ݿ��в�ѯ�� marking ��Ӧ���ļ�����·��
			fileType fileinfo;
			std::cout << "search the data from database������" << std::endl;
			bool b = MysqlMgr::GetInstance()->getFilePathFromMarking(marking,fileinfo);
			if (!b) {
				std::cout << "û���ҵ���� marking ��Ӧ���ļ� " << std::endl;
				rtvalue["error"] = 0;
				rtvalue["code"] = 10;
				std::string return_str = rtvalue.toStyledString();
				session->Send(return_str, iD_GET_FILE_FROM_MARKING_RSP);
				return;
			}


			fileinfo.filepath = fileinfo.filepath;
			std::cout << "the file path is : " << fileinfo.filepath << std::endl;


			// ��ʼ�����ļ����ͻ���
			std::wstring wpath = stringToWString(fileinfo.filepath);
			std::ifstream infile(wpath, std::ios::binary);
			if (!infile.is_open()) {
				std::cerr << "can not open the file...: " << fileinfo.filepath << std::endl;
				rtvalue["error"] = 0;
				rtvalue["code"] = 10;
				std::string return_str = rtvalue.toStyledString();
				session->Send(return_str, iD_GET_FILE_FROM_MARKING_RSP);
				return;
			}
			std::cout << "file open success!, begin to send file" << std::endl;

			const int BUFFER_SIZE = 4096;
			
			int64_t file_size = fileinfo.filesize;                           /// �ļ����ֽ���
			int64_t total_req = (file_size + BUFFER_SIZE - 1) / BUFFER_SIZE; /// �ļ��ܰ���

			std::cout << "file_size is : " << file_size;
			std::cout << "total_req is : " << total_req;

			char buffer[BUFFER_SIZE];
			int current_req = 1;

			while (infile && current_req <= total_req) {
				infile.read(buffer, BUFFER_SIZE);
				std::streamsize bytesRead = infile.gcount();

				// �������Ķ���������ת�� base64 ����
				std::string base64_data = base64_encode(reinterpret_cast<const unsigned char*>(buffer), bytesRead);

				Json::Value file_packet;
				file_packet["error"] = 0;
				file_packet["req"] = std::to_string(current_req);         // �ַ�����ʽ
				file_packet["total_req"] = std::to_string(total_req);     // �ַ�����ʽ
				file_packet["data"] = base64_data;
				file_packet["last"] = (current_req == total_req);         // bool ����
				file_packet["marking"] = marking;
				file_packet["filename"] = fileinfo.filename;
				file_packet["code"] = 0;

				std::string json_str = file_packet.toStyledString();
				session->Send(json_str, iD_GET_FILE_FROM_MARKING_RSP);

				current_req++;
			}

			infile.close();
			std::cout << "file send success! the file name : " << fileinfo.filename << std::endl;

		};
}

void LogicWorker::task_callback(std::shared_ptr<LogicNode> task)
{
	cout << "recv_msg id  is " << task->_recvnode->_msg_id << endl;
	auto call_back_iter = _fun_callbacks.find(task->_recvnode->_msg_id);
	if (call_back_iter == _fun_callbacks.end()) {
		return;
	}
	call_back_iter->second(task->_session, task->_recvnode->_msg_id,
		std::string(task->_recvnode->_data, task->_recvnode->_cur_len));
}
