#ifndef SINGLETON_H
#define SINGLETON_H

#include <global.h>
#include <memory>
#include <mutex>

// template <typename T>
// class Singleton {
// protected:
//     Singleton() = default;
//     ~Singleton() = default;

// public:
//     Singleton(const Singleton<T>&) = delete;
//     Singleton& operator=(const Singleton<T>&) = delete;

//     static T& getInstance() {
//         static T instance;
//         return instance;
//     }
// };

/**************************************************************/

//   * @file:      singleton.h
//   * @brife:     记录单例模板类遇到的一个错误：
/*
在此之前，我的服务端和客户端的单例基类模板的实现都是使用静态局部变量的方式实现的。
在客户端使用httpMge.getInstance().PostHttpReq()时，Qt报出错误：
“terminate called after throwing an instance of 'std::bad_weak_ptr' ，what():  bad_weak_ptr”。
在寻找了一番后发现了原因：
在PostHttpReq函数中使用了shared_from_this 获取指向自身的智能指针，
但是这个函数有个前提就是 enable_shared_from_this 要求对象必须由 std::shared_ptr 管理，使用局部静态变量的方式不满足这个条件。

为什么在服务器中使用静态局部变量的单例基类模板没有引发错误呢，因为无论是CServer还是httpConnection,在它们中使用shared_from_this()
之前都已经将它们的实例化对象使用 share_ptr 包裹了，因此没有引发这个问题。
*/

//   * @date:      2025/03/26

/**************************************************************/


template<typename T>
class Singleton{
protected:
    Singleton() = default;
    ~Singleton() = default;
    static std::shared_ptr<T> _instance;

public:
    Singleton(const Singleton<T>&) = delete;
    Singleton& operator = (const Singleton<T>& ) =delete;
    static std::shared_ptr<T> getInstance(){
        static std::once_flag flag;
        std::call_once(flag, [&](){
            _instance = std::shared_ptr<T>(new T);
        });
        return _instance;
    }
};

template<typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;


#endif // SINGLETON_H
