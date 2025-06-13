

#include "LogicSystem.h"
#include <csignal>
#include <thread>
#include <mutex>
#include "AsioIOServicePool.h"
#include "CServer.h"
#include "ConfigMgr.h"
#include <boost/filesystem.hpp>


using namespace std;
bool bstop = false;
std::condition_variable cond_quit;
std::mutex mutex_quit;



int main(){
	system("chcp 65001");
	SetConsoleOutputCP(CP_UTF8);  // 确保 Windows 控制台按 UTF-8 输出

	auto& cfg = ConfigMgr::Inst();
	auto server_name = cfg["SelfServer"]["Name"];

	try {
		auto pool = AsioIOServicePool::GetInstance();

		boost::asio::io_context  io_context;
		boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
		signals.async_wait([&io_context, pool](auto, auto) {
			io_context.stop();
			pool->Stop();
			});
		auto port_str = cfg["SelfServer"]["Port"];
		CServer s(io_context, atoi(port_str.c_str()));
		io_context.run();
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << endl;
	}

}


