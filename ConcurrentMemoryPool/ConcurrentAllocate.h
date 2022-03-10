#pragma once
#include "ThreadCache.h"
#include "PageCache.h"
#include "ObjectPool.h"
#include "PageMap.h"

static ObjectPool<ThreadCache> tcPool;

//为什么需要创建下面2个函数?/为什么还要封装一层?
//因为每个线程都有自己的TLS，我们不可能让用户自己去调用TLS然后才能调到Allocate，而是应该直接给他们提供接口。让他们使用接口就行了

//封装一层
 static void* ConcurrentAllocate(size_t size)	//对于tcmalloc中，这里的的名称就是tcmalloc了
{
	//当对象大小 > 256KB时 (大于NAPES - 1的情况放到NewSpan里面处理)
	if (size > MAX_BYTES)
	{
		size_t alignSize = SizeClass::RoundUp(size);
		PageCache::GetInstance()->PageLock();
		Span* span = PageCache::GetInstance()->NewSpan(alignSize >> PAGE_SHIFT);
		PageCache::GetInstance()->PageUnLock();
		void* ptr = (void*)(span->_pageID << PAGE_SHIFT);//span当中的_pageID和_n是管理开辟出来的空间的字段
		return ptr;
	}
	else
	{
		if (pTLSThreadCache == nullptr)
		{
			//pTLSThreadCache = new ThreadCache;
			//static ObjectPool<ThreadCache> tcPool;
 			pTLSThreadCache = tcPool.New();
		}
		return pTLSThreadCache->Allocate(size);
	}
}

//封装一层
 static void ConcurrentFree(void* ptr)
{
	 //Free时
	 PAGE_ID pageID = ((PAGE_ID)ptr >> PAGE_SHIFT);
	 //PageCache::GetInstance()->PageLock();	//别人可能正在对_idSpanMap进行写入操作, 你应该等别人写完再读(别人写入的操作只在NewSpan函数中, 而调用NewSpan函数前都会加锁)
	 //Span* span = PageCache::GetInstance()->MapPAGEIDToSpan(pageID);
	 //PageCache::GetInstance()->PageUnLock();

	 //使用基数树优化后不需要加锁访问了
	 Span* span = PageCache::GetInstance()->MapPAGEIDToSpan(pageID);

	 size_t size = span->_ObjectSize;	//通过Span当中的ObjectSize获取对象大小

	if (size > MAX_BYTES)//(大于NAPES - 1的情况放到ReleaseSpanToPageCache里面处理)
	{
		PageCache::GetInstance()->PageLock();
		PageCache::GetInstance()->ReleaseSpanToPageCache(span);
		PageCache::GetInstance()->PageUnLock();
	}
	else
	{
		assert(pTLSThreadCache);
		pTLSThreadCache->Deallocate(ptr, size);
	}
}