#pragma once
#include <memory>
#include <mutex>

template<typename T>
class Singleton{
protected:
	Singleton() = default;
	~Singleton() = default;
public:
	Singleton(const Singleton&) = delete;
	Singleton& operator =(const Singleton&) = delete;
	static T& getInstance() {
		static T instance;
		return instance;
	}
};

template<typename T>
class SingletonPtr {
protected:
	SingletonPtr() = default;
	
	static std::shared_ptr<T> _instance;
	static std::once_flag _flag;

	SingletonPtr(const SingletonPtr<T>&) = delete;
	SingletonPtr& operator =(const SingletonPtr<T>&) = delete;
	
public:
	~SingletonPtr() = default;
	static std::shared_ptr<T> getInstance() {
		std::call_once(_flag, [&]() {
			_instance = std::shared_ptr<T>(new T());
			//_instance = std::make_shared<T>();
			});
		return _instance;
	}
};



template<typename T>
std::shared_ptr<T> SingletonPtr<T>::_instance = nullptr;

template<typename T>
std::once_flag SingletonPtr<T>::_flag;

