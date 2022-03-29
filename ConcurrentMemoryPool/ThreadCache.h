#pragma once
#include "Common.h"

class ThreadCache
{
public:
	//申请和释放内存对象
	void* Allocate(size_t size);
	void Deallocate(void* ptr, size_t size);

	void ListTooLong(FreeList& freelist, size_t size);

	//从CentralCache获取alignNum对象大小的小块内存
	void* FetchFromCentralCache(size_t Index, size_t alignNum);
	~ThreadCache();
private:
	FreeList _freelists[NFREELISTS];
};

// TLS thread local storage
#ifdef _WIN32
static _declspec(thread) ThreadCache* pTLSThreadCache = nullptr;//必须加上static，不然会在多个.cpp里面重定义
#else //linux
static __thread ThreadCache* pTLSThreadCache = nullptr;
#endif
