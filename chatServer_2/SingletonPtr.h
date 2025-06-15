#pragma once
#include <memory>
#include <mutex>

template<typename T>
class SingletonPtr{
protected:
	SingletonPtr() = default;
	static std::once_flag _flag;
	static std::shared_ptr<T> _instance;
public:
	~SingletonPtr() = default;
	SingletonPtr(const SingletonPtr<T>&) = delete;
	SingletonPtr& operator = (const SingletonPtr<T>&) = delete;
	static std::shared_ptr<T> getInstance() {
		std::call_once(_flag, [&]() {
			_instance = std::shared_ptr<T>(new T());
			});
		return _instance;
	}
};

template<typename T>
std::shared_ptr<T> SingletonPtr<T>::_instance = nullptr;

template <typename T>
std::once_flag SingletonPtr<T>::_flag;

