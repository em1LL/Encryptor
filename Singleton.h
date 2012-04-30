#ifndef SINGLETON_H
#define SINGLETON_H

template<typename T>
class Singleton
{
public:
	virtual ~Singleton() {};
	static T* getInstance();

protected:
	Singleton() {};
	static T* singletonInstance;
};

template<typename T>
T *Singleton <T>::getInstance()
{
	if (!singletonInstance)
	{
		singletonInstance = new T();
	}

	return singletonInstance;
}

template<typename T>
T *Singleton<T>::singletonInstance;

#endif // SINGLETON_H