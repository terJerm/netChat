#pragma once
#include "const.h"
#include <hiredis.h>       //redis �� ͷ�ļ�
#include "Singleton.h"
#include <queue>

class RedisConPool;


/*
	���� redis ���࣬�ڲ�ʹ�����ӳ�
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

	// redis �ֲ�ʽ��
	std::string acquireLock(const std::string& lockName,
		int lockTimeout, int acquireTimeout);      // ����
	bool releaseLock(const std::string& lockName,
		const std::string& identifier);            // ����

	void DecreaseCount(std::string server_name);
};

/*
	����ĵ������Ӳ������̰߳�ȫ�ģ�������̲߳�����ʱ���ܳ��ִ���
	ʹ�� redis ���ӳ� �Ż�,�õ���_connect ���һ�����ӳ�
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
