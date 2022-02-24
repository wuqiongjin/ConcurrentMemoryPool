#pragma once
#include "ThreadCache.h"

//Ϊʲô��Ҫ��������2������?/Ϊʲô��Ҫ��װһ��?
//��Ϊÿ���̶߳����Լ���TLS�����ǲ��������û��Լ�ȥ����TLSȻ����ܵ���Allocate������Ӧ��ֱ�Ӹ������ṩ�ӿڡ�������ʹ�ýӿھ�����

//��װһ��
static void* ConcurrentAllocate(size_t size)	//����tcmalloc�У�����ĵ����ƾ���tcmalloc��
{
	if (pTLSThreadCache == nullptr)
	{
		pTLSThreadCache = new ThreadCache;
	}
	cout << std::this_thread::get_id() << ":" << pTLSThreadCache << endl;
	return pTLSThreadCache->Allocate(size);
}

//��װһ��
static void ConcurrentDeallocate(void* ptr, size_t size)
{
	assert(pTLSThreadCache);
	pTLSThreadCache->Deallocate(ptr, size);
}