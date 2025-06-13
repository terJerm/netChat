#pragma once

#include <boost/asio.hpp>
#include <vector>
#include "SingletonPtr.h"
#include <memory>
#include <thread>
#include <iostream>

class AsioIOServicePool:public SingletonPtr<AsioIOServicePool>{
	friend class SingletonPtr<AsioIOServicePool>;
	using servicePtr = std::shared_ptr<boost::asio::io_context>;
	using serviceGuard = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
	using threadUniquePtr = std::unique_ptr<std::thread>;


private:
	std::vector<servicePtr> io_serv_;
	std::vector<serviceGuard> work_guard_;
	std::vector<threadUniquePtr> threads_;

	bool initialized;
	std::atomic<size_t> next_to_index_;
	std::mutex mutex_;


private:
	AsioIOServicePool();
	void init(std::size_t thread_count = std::thread::hardware_concurrency());
public:
	~AsioIOServicePool();
	AsioIOServicePool(const AsioIOServicePool&) = delete;
	AsioIOServicePool& operator = (const AsioIOServicePool&) = delete;
	void Stop();
	void resize(size_t new_count);
	boost::asio::io_context& getIOContext();


};

