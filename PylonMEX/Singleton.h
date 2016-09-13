#pragma once
#define SINGLETON_DEFINITION(T) \
public: \
	static T & Instance(void) \
{ \
return mSingleton; \
} \
static T * InstancePtr(void) \
{ \
return &mSingleton; \
} \
protected: \
	static T mSingleton; \
private: \
	T(T const &) {} \
	T& operator = (T const &) { return *this; }

#define SINGLETON_IMPLEMENTATION(T)\
		T T::mSingleton = T();