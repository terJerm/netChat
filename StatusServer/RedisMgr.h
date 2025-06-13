#pragma once
#include "const.h"
#include <hiredis.h>       //redis 库 头文件
#include "Singleton.h"
#include <queue>

class RedisConPool;


/*
	访问 redis 的类，内部使用连接池
*/

class RedisMgr:public SingletonPtr<RedisMgr>{
	friend class SingletonPtr<RedisMgr>;
private:
	RedisMgr();
	std::unique_ptr<RedisConPool> _con_pool;
	
public:
	~RedisMgr();
	RedisMgr(const RedisMgr&) = delete;
	RedisMgr& operator=(const RedisMgr&) = delete;

	bool Get(const std::string& key, std::string& value);
	bool Set(const std::string& key, const std::string& value);
	bool Auth(const std::string& password);
	bool LPush(const std::string& key, const std::string& value);
	bool LPop(const std::string& key, std::string& value);
	bool RPush(const std::string& key, const std::string& value);
	bool RPop(const std::string& key, std::string& value);
	bool HSet(const std::string& key, const std::string& hkey, const std::string& value);
	bool HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen);
	std::string HGet(const std::string& key, const std::string& hkey);
	bool Del(const std::string& key);
	bool HDel(const std::string& key, const std::string& field);
	bool ExistsKey(const std::string& key);
	void Close();

	// redis 分布式锁
	std::string acquireLock(const std::string& lockName,
		int lockTimeout, int acquireTimeout);      // 加锁
	bool releaseLock(const std::string& lockName,
		const std::string& identifier);            // 解锁

	void DecreaseCount(std::string server_name);
};

/*
	上面的单例连接并不是线程安全的，当多个线程操作它时可能出现错误
	使用 redis 连接池 优化,让单个_connect 变成一个连接池
*/

class RedisConPool {
private:
	std::atomic<bool> _stop;
	std::mutex _mutex;
	std::condition_variable _cond;
	std::size_t _pool_size;
	const char* _host;
	int _port;
	std::queue<redisContext* > _connects;
public:
	RedisConPool(std::size_t size,const char* host,int port,const char* password);
	~RedisConPool();
	void close();
	void returnConnection(redisContext* context);
	redisContext* getConnection();

};
