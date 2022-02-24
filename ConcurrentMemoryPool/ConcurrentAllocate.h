#pragma once
#include "ThreadCache.h"

//为什么需要创建下面2个函数?/为什么还要封装一层?
//因为每个线程都有自己的TLS，我们不可能让用户自己去调用TLS然后才能调到Allocate，而是应该直接给他们提供接口。让他们使用接口就行了

//封装一层
static void* ConcurrentAllocate(size_t size)	//对于tcmalloc中，这里的的名称就是tcmalloc了
{
	if (pTLSThreadCache == nullptr)
	{
		pTLSThreadCache = new ThreadCache;
	}
	cout << std::this_thread::get_id() << ":" << pTLSThreadCache << endl;
	return pTLSThreadCache->Allocate(size);
}

//封装一层
static void ConcurrentDeallocate(void* ptr, size_t size)
{
	assert(pTLSThreadCache);
	pTLSThreadCache->Deallocate(ptr, size);
}