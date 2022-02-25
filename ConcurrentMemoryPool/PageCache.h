#pragma once
#include "Common.h"

//����ģʽPageCache
class PageCache
{
public:
	static PageCache* GetInstance()
	{
		return &_sInst;
	}

	//�������һ��"ҳ��"��СΪ"NPAGES - 1"���µ�Span
	Span* NewSpan(size_t page);

	void PageLock() { _pageMutex.lock(); }
	void PageUnLock() { _pageMutex.unlock(); }
private:
	PageCache(){}
	PageCache(const PageCache&) = delete;
	PageCache& operator=(const PageCache&) = delete;

	static PageCache _sInst;
	std::mutex _pageMutex;	//һ�Ѵ�����һ������PageCache��Ҫ����

private:
	SpanList _spanlists[NPAGES];	//��ҳ��Ϊӳ��Ĺ���(ֱ�Ӷ�ַ��)
};