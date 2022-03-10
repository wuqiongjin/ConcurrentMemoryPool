#pragma once
#include "ThreadCache.h"
#include "PageCache.h"
#include "ObjectPool.h"
#include "PageMap.h"

static ObjectPool<ThreadCache> tcPool;

//Ϊʲô��Ҫ��������2������?/Ϊʲô��Ҫ��װһ��?
//��Ϊÿ���̶߳����Լ���TLS�����ǲ��������û��Լ�ȥ����TLSȻ����ܵ���Allocate������Ӧ��ֱ�Ӹ������ṩ�ӿڡ�������ʹ�ýӿھ�����

//��װһ��
 static void* ConcurrentAllocate(size_t size)	//����tcmalloc�У�����ĵ����ƾ���tcmalloc��
{
	//�������С > 256KBʱ (����NAPES - 1������ŵ�NewSpan���洦��)
	if (size > MAX_BYTES)
	{
		size_t alignSize = SizeClass::RoundUp(size);
		PageCache::GetInstance()->PageLock();
		Span* span = PageCache::GetInstance()->NewSpan(alignSize >> PAGE_SHIFT);
		PageCache::GetInstance()->PageUnLock();
		void* ptr = (void*)(span->_pageID << PAGE_SHIFT);//span���е�_pageID��_n�ǹ����ٳ����Ŀռ���ֶ�
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

//��װһ��
 static void ConcurrentFree(void* ptr)
{
	 //Freeʱ
	 PAGE_ID pageID = ((PAGE_ID)ptr >> PAGE_SHIFT);
	 //PageCache::GetInstance()->PageLock();	//���˿������ڶ�_idSpanMap����д�����, ��Ӧ�õȱ���д���ٶ�(����д��Ĳ���ֻ��NewSpan������, ������NewSpan����ǰ�������)
	 //Span* span = PageCache::GetInstance()->MapPAGEIDToSpan(pageID);
	 //PageCache::GetInstance()->PageUnLock();

	 //ʹ�û������Ż�����Ҫ����������
	 Span* span = PageCache::GetInstance()->MapPAGEIDToSpan(pageID);

	 size_t size = span->_ObjectSize;	//ͨ��Span���е�ObjectSize��ȡ�����С

	if (size > MAX_BYTES)//(����NAPES - 1������ŵ�ReleaseSpanToPageCache���洦��)
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