#include "AsioIOServerPool.h"
#include <exception>


AsioIOServerPool::AsioIOServerPool():_next_index(0), _initiallized(false){
	std::cout << "This AsioIOServerPool is already constructed!..." << std::endl;
	init();
}
AsioIOServerPool::~AsioIOServerPool() {
	stop();
	std::cout << "This AsioIOServerPool has been deconstructed!..." << std::endl;
}
void AsioIOServerPool::stop() {
	std::lock_guard<std::mutex>lock(_mutex);
	if (!_initiallized) return;
	for (auto& io : _servers) {
		io->stop();
	}
	for (auto& t : _threads) {
		if (t->joinable()) t->join();
	}
	_servers.clear();
	_guards.clear();
	_threads.clear();
	_initiallized = false;
	std::cout << "IOServcePool stopped" << std::endl;

}
void AsioIOServerPool::init(std::size_t thread_num) {
	std::lock_guard<std::mutex>lock(_mutex);
	if (_initiallized) return;
	for (std::size_t q = 0; q < thread_num; q++) {
		auto io = std::make_shared<boost::asio::io_context>();
		_servers.push_back(io);
		_guards.emplace_back(boost::asio::make_work_guard(*io));
	}
	for (auto& io : _servers) {
		_threads.emplace_back(std::make_unique<std::thread>([io]() {
			try {
				io->run();
			}
			catch (const std::exception& ec) {
				std::cerr << "threads failed to run: " << ec.what() << std::endl;
			}
			}));
	}
	_initiallized = true;
	std::cout << "the IOServerPool initialized success" << std::endl;
}
void AsioIOServerPool::resize(std::size_t new_count) {
	std::lock_guard<std::mutex> lock(_mutex);
	if (new_count == _servers.size()) return;
	stop();
	init(new_count);
	std::cout << "[IOServicePool] Resized to " << new_count << " threads.\n";
}
AsioIOServerPool::ioSharedPtr& AsioIOServerPool::getIOContext() {
	return _servers[_next_index.fetch_add(1, std::memory_order_relaxed) % _servers.size()];
}
