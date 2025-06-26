#include "AsioIOServicePool.h"

AsioIOServicePool::AsioIOServicePool():next_to_index_(0),initialized(false) {
	init();
}

AsioIOServicePool::~AsioIOServicePool() {
	Stop();
}

void AsioIOServicePool::init(std::size_t thread_count) {
	std::lock_guard<std::mutex>lock(mutex_);
	if (initialized) return;
	//最大线程数为 CPU 核心数，最小值为2
	thread_count = std::max<std::size_t>(2, thread_count-1);
	//初始化 ioVector 和它的 eordGuard
	for (std::size_t i = 0; i < thread_count; ++i) {
		auto io = std::make_shared<boost::asio::io_context>();
		io_serv_.push_back(io);
		work_guard_.emplace_back(boost::asio::make_work_guard(*io));
	}
	//初始化线程池：每个线程分配一个 io 并 run()
	for (auto& io : io_serv_) {
		threads_.emplace_back(std::make_unique<std::thread>([io]() {
			try {
				io->run();
			}
			catch (const std::exception& e) {
				std::cerr << "thread exception: " << e.what() << std::endl;
			}}));
	}
	initialized = true;
	std::cout << "IOServicePool initialized with " << thread_count << std::endl;
}
void AsioIOServicePool::Stop() {
	std::lock_guard<std::mutex>lock(mutex_);
	if (!initialized) return;
	for (auto& io : io_serv_) {
		io->stop();
	}
	for (auto& t : threads_) {
		if (t->joinable()) t->join();
	}
	io_serv_.clear();
	work_guard_.clear();
	threads_.clear();
	initialized = false;
	std::cout << "IOServcePool stopped" << std::endl;

}
void AsioIOServicePool::resize(size_t new_count) {
	std::lock_guard<std::mutex> lock(mutex_);
	if (new_count == io_serv_.size()) return;
	Stop();
	init(new_count);
	std::cout << "[IOServicePool] Resized to " << new_count << " threads.\n";
}
boost::asio::io_context& AsioIOServicePool::getIOContext() {
	return *io_serv_[next_to_index_.fetch_add(1, std::memory_order_relaxed) % io_serv_.size()];
}
