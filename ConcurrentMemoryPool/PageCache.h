#pragma once
#include "Common.h"

//单例模式PageCache
class PageCache
{
public:
	static PageCache* GetInstance()
	{
		return &_sInst;
	}

	//向堆申请一个"页数"大小为"NPAGES - 1"的新的Span
	Span* NewSpan(size_t page);

	void PageLock() { _pageMutex.lock(); }
	void PageUnLock() { _pageMutex.unlock(); }
private:
	PageCache(){}
	PageCache(const PageCache&) = delete;
	PageCache& operator=(const PageCache&) = delete;

	static PageCache _sInst;
	std::mutex _pageMutex;	//一把大锁，一旦访问PageCache就要加锁

private:
	SpanList _spanlists[NPAGES];	//以页数为映射的规则(直接定址法)
};