#pragma once
#include "Common.h"

class ThreadCache
{
public:
	//������ͷ��ڴ����
	void* Allocate(size_t size);
	void Deallocate(void* ptr, size_t size);

	void ListTooLong(FreeList& freelist, size_t size);

	//��CentralCache��ȡalignNum�����С��С���ڴ�
	void* FetchFromCentralCache(size_t Index, size_t alignNum);
	~ThreadCache();
private:
	FreeList _freelists[NFREELISTS];
};

// TLS thread local storage
#ifdef _WIN32
static _declspec(thread) ThreadCache* pTLSThreadCache = nullptr;//�������static����Ȼ���ڶ��.cpp�����ض���
#else //linux
static __thread ThreadCache* pTLSThreadCache = nullptr;
#endif
