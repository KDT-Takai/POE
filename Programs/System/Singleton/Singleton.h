#pragma once

//	シングルトン基底クラス
template <typename T>
class Singleton {
protected:
	Singleton() = default;
	virtual ~Singleton() = default;
public:
	Singleton(const Singleton&) = delete;
	Singleton& operator=(const Singleton&) = delete;

	static T& Instance()
	{
		static T instance;
		return instance;
	}
};