#pragma once
#include "Common.h"

class ThreadCache
{
public:
	//������ͷ��ڴ����
	void* Allocate(size_t size);
	void Deallocate(void* ptr, size_t size);

	void* FetchFromCentralCache(size_t Index, size_t alignNum);

private:
	FreeList _freelists[NFREELISTS];
};

// TLS thread local storage
static _declspec(thread) ThreadCache* pTLSThreadCache = nullptr;//�������static����Ȼ���ڶ��.cpp�����ض���