#pragma once
#include <vector>
#include "Singleton.h"
#include <boost/asio.hpp>
#include <memory>
#include <atomic>
#include <thread>
#include <iostream>
#include <boost/asio/executor_work_guard.hpp>


/*
	���߳� io_context ��߲�������������̣߳�ÿ���߳��д���һ��server���е���
*/



class AsioIOServerPool:public SingletonPtr<AsioIOServerPool>{
	friend class SingletonPtr<AsioIOServerPool>;

	using ioSharedPtr = std::shared_ptr<boost::asio::io_context>;
	using IOGuard =
		boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
	using threadUniquePtr = std::unique_ptr<std::thread>;
	
private:
	AsioIOServerPool();
	std::vector<ioSharedPtr> _servers;
	std::vector<IOGuard> _guards;
	std::vector<threadUniquePtr> _threads;
	std::mutex _mutex;
	std::atomic<std::size_t> _next_index;
	bool _initiallized;

public:
	~AsioIOServerPool();
	AsioIOServerPool(const AsioIOServerPool&) = delete;
	AsioIOServerPool& operator = (const AsioIOServerPool&) = delete;
	void stop();
	void init(std::size_t thread_num = std::thread::hardware_concurrency() - 1);
	void resize(std::size_t new_count);
	ioSharedPtr& getIOContext();
};

