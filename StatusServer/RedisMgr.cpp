#include "RedisMgr.h"
#include "configMgr.h"
#include "defer.h"
#include "DistLock.h"


RedisMgr::RedisMgr() {
    auto& gCfgMgr = ConfigMgr::getInstance();
    auto host = (gCfgMgr)["Redis"]["Host"];
    auto port = (gCfgMgr)["Redis"]["Port"];
    auto pwd = (gCfgMgr)["Redis"]["Passwd"];
    _con_pool.reset(new RedisConPool(5, host.c_str(), atoi(port.c_str()), pwd.c_str()));
}

RedisMgr::~RedisMgr() {
    Close();
}


bool RedisMgr::Get(const std::string& key, std::string& value) {

    //std::cout << "inter there——2" << std::endl;

    auto connect = _con_pool->getConnection();
    if (connect == nullptr) {
        //std::cout << "inter there————4" << std::endl;
        _con_pool->returnConnection(connect);
        return false;
    }
	auto reply = (redisReply*)redisCommand(connect, "GET %s", key.c_str());
    if (reply == NULL) {
        std::cout << "[ GET  " << key << " ] failed" << std::endl;
        //freeReplyObject(reply);       //释放redisCommand执行后返回的redisReply所占用的内存
        _con_pool->returnConnection(connect);
        return false;
    }
    if (reply->type != REDIS_REPLY_STRING) {
        std::cout << "[ GET  " << key << " ] failed" << std::endl;
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }
    value = reply->str;
    freeReplyObject(reply);

    std::cout << "Succeed to execute command [ GET " << key << "  ]" << std::endl;

    _con_pool->returnConnection(connect);
    return true;
}

bool RedisMgr::Set(const std::string& key, const std::string& value) {
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) {
        _con_pool->returnConnection(connect);
        return false;
    }
    auto reply = (redisReply*)redisCommand(connect, "SET %s %s", key.c_str(), value.c_str());
    //如果返回NULL则说明执行失败
    if (NULL == reply){
        std::cout << "Execut command [ SET " << key << "  " << value << " ] failure ! " << std::endl;
        //freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }
    //如果执行失败则释放连接
    if (!(reply->type == REDIS_REPLY_STATUS && (strcmp(reply->str, "OK") == 0 || 
        strcmp(reply->str, "ok") == 0))){
        std::cout << "Execut command [ SET " << key << "  " << value << " ] failure ! " << std::endl;
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }
    //执行成功 释放redisCommand执行后返回的redisReply所占用的内存
    freeReplyObject(reply);
    std::cout << "Execut command [ SET " << key << "  " << value << " ] success ! " << std::endl;

    _con_pool->returnConnection(connect);
    return true;
}

//认证登录
bool RedisMgr::Auth(const std::string& password) {
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) {
        _con_pool->returnConnection(connect);
        return false;
    }
    auto reply = (redisReply*)redisCommand(connect, "AUTH %s", password.c_str());
    if (reply->type == REDIS_REPLY_ERROR) {
        std::cout << "认证失败" << std::endl;
        //执行成功 释放redisCommand执行后返回的redisReply所占用的内存
        freeReplyObject(reply);

        _con_pool->returnConnection(connect);
        return false;
    }
    else {
        //执行成功 释放redisCommand执行后返回的redisReply所占用的内存
        freeReplyObject(reply);
        std::cout << "认证成功" << std::endl;
        _con_pool->returnConnection(connect);
        return true;
    }
}

bool RedisMgr::LPush(const std::string& key, const std::string& value) {
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) {
        _con_pool->returnConnection(connect);
        return false;
    }
    auto reply = (redisReply*)redisCommand(connect, "LPUSH %s %s", key.c_str(), value.c_str());
    if (NULL == reply){
        std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] failure ! " << std::endl;
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }
    if (reply->type != REDIS_REPLY_INTEGER || reply->integer <= 0) {
        std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] failure ! " << std::endl;
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }
    std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] success ! " << std::endl;
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

bool RedisMgr::LPop(const std::string& key, std::string& value) {
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) {
        _con_pool->returnConnection(connect);
        return false;
    }
    auto reply = (redisReply*)redisCommand(connect, "LPOP %s ", key.c_str());
    if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
        std::cout << "Execut command [ LPOP " << key << " ] failure ! " << std::endl;
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }
    value = reply->str;
    std::cout << "Execut command [ LPOP " << key << " ] success ! " << std::endl;
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

bool RedisMgr::RPush(const std::string& key, const std::string& value) {
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) {
        _con_pool->returnConnection(connect);
        return false;
    }
    auto reply = (redisReply*)redisCommand(connect, "RPUSH %s %s", key.c_str(), value.c_str());
    if (NULL == reply){
        std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] failure ! " << std::endl;
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }
    if (reply->type != REDIS_REPLY_INTEGER || reply->integer <= 0) {
        std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] failure ! " << std::endl;
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }
    std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] success ! " << std::endl;
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

bool RedisMgr::RPop(const std::string& key, std::string& value) {
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) {
        _con_pool->returnConnection(connect);
        return false;
    }
    auto reply = (redisReply*)redisCommand(connect, "RPOP %s ", key.c_str());
    if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
        std::cout << "Execut command [ RPOP " << key << " ] failure ! " << std::endl;
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }
    value = reply->str;
    std::cout << "Execut command [ RPOP " << key << " ] success ! " << std::endl;
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

bool RedisMgr::HSet(const std::string& key, const std::string& hkey, const std::string& value) {
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) {
        std::cout << "HSet: Failed to get Redis connection!" << std::endl;
        _con_pool->returnConnection(connect);
        return false;
    }

    auto reply = (redisReply*)redisCommand(connect, "HSET %s %s %s", key.c_str(), hkey.c_str(), value.c_str());

    if (reply == nullptr) {
        std::cout << "HSet: redisCommand returned null!" << std::endl;
        _con_pool->returnConnection(connect);
        return false;
    }

    // 添加这段调试输出
    std::cout << "HSet: reply->type = " << reply->type
        << ", reply->str = " << (reply->str ? reply->str : "null")
        << ", reply->integer = " << reply->integer << std::endl;

    if (reply->type != REDIS_REPLY_INTEGER) {
        std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << value << " ] failure ! " << std::endl;
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }

    std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << value << " ] success ! " << std::endl;
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

bool RedisMgr::HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen){
    const char* argv[4];
    size_t argvlen[4];
    argv[0] = "HSET";
    argvlen[0] = 4;
    argv[1] = key;
    argvlen[1] = strlen(key);
    argv[2] = hkey;
    argvlen[2] = strlen(hkey);
    argv[3] = hvalue;
    argvlen[3] = hvaluelen;

    auto connect = _con_pool->getConnection();
    if (connect == nullptr) {
        _con_pool->returnConnection(connect);
        return false;
    }
    auto reply = (redisReply*)redisCommandArgv(connect, 4, argv, argvlen);
    if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER) {
        std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue << " ] failure ! " << std::endl;
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }
    std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue << " ] success ! " << std::endl;
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

std::string RedisMgr::HGet(const std::string& key, const std::string& hkey){
    const char* argv[3];
    size_t argvlen[3];
    argv[0] = "HGET";
    argvlen[0] = 4;
    argv[1] = key.c_str();
    argvlen[1] = key.length();
    argv[2] = hkey.c_str();
    argvlen[2] = hkey.length();

    auto connect = _con_pool->getConnection();
    if (connect == nullptr) {
        _con_pool->returnConnection(connect);
        return "";
    }
    auto reply = (redisReply*)redisCommandArgv(connect, 3, argv, argvlen);
    if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
        freeReplyObject(reply);
        std::cout << "Execut command [ HGet " << key << " " << hkey << "  ] failure ! " << std::endl;
        _con_pool->returnConnection(connect);
        return "";
    }
    std::string value = reply->str;
    freeReplyObject(reply);
    std::cout << "Execut command [ HGet " << key << " " << hkey << " ] success ! " << std::endl;
    _con_pool->returnConnection(connect);
    return value;
}

bool RedisMgr::Del(const std::string& key){
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) {
        _con_pool->returnConnection(connect);
        return false;
    }
    auto reply = (redisReply*)redisCommand(connect, "DEL %s", key.c_str());
    if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER) {
        std::cout << "Execut command [ Del " << key << " ] failure ! " << std::endl;
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }
    std::cout << "Execut command [ Del " << key << " ] success ! " << std::endl;
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

bool RedisMgr::ExistsKey(const std::string& key){
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) {
        _con_pool->returnConnection(connect);
        return false;
    }
    auto reply = (redisReply*)redisCommand(connect, "exists %s", key.c_str());
    if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER || reply->integer == 0) {
        std::cout << "Not Found [ Key " << key << " ]  ! " << std::endl;
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }
    std::cout << " Found [ Key " << key << " ] exists ! " << std::endl;
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

void RedisMgr::Close(){
    _con_pool->close();
}

// 加锁操作
std::string RedisMgr::acquireLock(const std::string& lockName, int lockTimeout, int acquireTimeout)
{
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) {
        //_con_pool->returnConnection(connect);
        return "";
    }
    Defer defer([&connect,this]() {
        _con_pool->returnConnection(connect);
        });

    return DistLock::Inst().acquireLock(connect, lockName, lockTimeout, acquireTimeout);
}

// 解锁操作
bool RedisMgr::releaseLock(const std::string& lockName, const std::string& identifier)
{
    if (identifier.empty()) {
        return true;
    }
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) {
        //_con_pool->returnConnection(connect);
        return false;
    }
    Defer defer([&connect,this]() {
        _con_pool->returnConnection(connect);
        });

    return DistLock::Inst().releaseLock(connect, lockName, identifier);
}

void RedisMgr::DecreaseCount(std::string server_name)
{
    auto lock_key = LOCK_COUNT;
    auto identifier = RedisMgr::getInstance()->acquireLock(lock_key, LOCK_TIME_OUT, ACQUIRE_TIME_OUT);
   
    Defer defer2([this, identifier, lock_key]() {
        RedisMgr::getInstance()->releaseLock(lock_key, identifier);
        });

    
    auto rd_res = RedisMgr::getInstance()->HGet(LOGIN_COUNT, server_name);
    int count = 0;
    if (!rd_res.empty()) {
        count = std::stoi(rd_res);
        if (count > 0) {
            count--;
        }
    }

    auto count_str = std::to_string(count);
    RedisMgr::getInstance()->HSet(LOGIN_COUNT, server_name, count_str);
}


/*
    上面的单例连接并不是线程安全的，当多个线程操作它时可能出现错误
    使用 redis 连接池 优化
*/


RedisConPool::RedisConPool(std::size_t size, const char* host, int port, const char* password):
    _pool_size(size), _host(host), _port(port), _stop(false) {
    for (std::size_t q = 0; q < _pool_size; q++) {
        auto* connect = redisConnect(_host, _port);
        if (connect == nullptr || connect->err != 0) {
            if (connect != nullptr) redisFree(connect);
            std::cout << "redis_" << q << ": connect failed ..." << std::endl;
            continue;
        }

        std::cout << "redis_" << q << ": connect success..." << std::endl;

        redisReply* reply = (redisReply*)redisCommand(connect, "AUTH %s", password);
        if (reply == nullptr) {
            std::cout << "redis_" << q << ": AUTH reply is null!" << std::endl;
            redisFree(connect);
            continue;
        }

        // 检查是否为认证成功
        if (reply->type != REDIS_REPLY_STATUS || std::string(reply->str) != "OK") {
            std::cout << "redis_" << q << ": AUTH failed! type=" << reply->type
                << ", str=" << (reply->str ? reply->str : "null") << std::endl;
            freeReplyObject(reply);
            redisFree(connect);
            continue;
        }

        freeReplyObject(reply);
        std::cout << "redis_" << q << ": AUTH success" << std::endl;
        _connects.push(connect);
    }
}

RedisConPool::~RedisConPool() {
    std::lock_guard<std::mutex> lock(_mutex);
    while (!_connects.empty()) {
        redisFree(_connects.front());
        _connects.pop();
    }
}

void RedisConPool::close() {
    _stop = true;
    _cond.notify_all();
}

void RedisConPool::returnConnection(redisContext* context) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_stop) { return; }
    _connects.push(context);
    std::cout << "return one redis connection to pool..." << std::endl;
    _cond.notify_one();
}

redisContext* RedisConPool::getConnection() {
    std::unique_lock<std::mutex>lock(_mutex);
    _cond.wait(lock, [this] {           //判断队列中是否有空闲的连接决定挂起还是继续执行
        if (_stop) return true;
        return !_connects.empty();
        });
    if (_stop) return nullptr;
    auto* connect = _connects.front();
    _connects.pop();
    std::cout << "get one redis connection from pool..." << std::endl;
    return connect;
}

bool RedisMgr::HDel(const std::string& key, const std::string& field) {
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) {
        return false;
    }

    Defer defer([&connect, this]() {
        _con_pool->returnConnection(connect);
        });

    redisReply* reply = (redisReply*)redisCommand(connect, "HDEL %s %s", key.c_str(), field.c_str());
    if (reply == nullptr) {
        std::cerr << "HDEL command failed" << std::endl;
        return false;
    }

    bool success = false;
    if (reply->type == REDIS_REPLY_INTEGER) {
        success = reply->integer > 0;
    }

    freeReplyObject(reply);
    return success;
}