#pragma once
#include "Common.h"
#include "ObjectPool.h"
#include "PageMap.h"


//单例模式PageCache
class PageCache
{
public:
	static PageCache* GetInstance()
	{
    return &_sInst;
    //static std::mutex mtx;
    //if(_sInst == nullptr)
    //{
    //  mtx.lock();
    //  if(_sInst == nullptr)
    //  {
    //    cout << "new a PageCache()" << endl;
    //    _sInst = new PageCache;
    //    if(_sInst == nullptr)
    //      cout << "new PageCache() failed!" << endl;
    //    if(_sInst != nullptr)
    //      cout << "new PageCache() Success!" << endl;
    //  }
    //  mtx.unlock();
    //}
    //return _sInst;
	}

	//向堆申请一个"页数"大小为"NPAGES - 1"的新的Span
	Span* NewSpan(size_t page);

	//将PAGE_ID映射到一个Span*上, 这样可以通过页号直接找到对应的Span*的位置
	Span* MapPAGEIDToSpan(PAGE_ID id);

	//将useCount减为0的Span返回给PageCache，以用来合并成更大的Span
	void ReleaseSpanToPageCache(Span* span);

	void PageLock() { _pageMutex.lock(); }
	void PageUnLock() { _pageMutex.unlock(); }
private:
	PageCache() {}
	PageCache(const PageCache&) = delete;
	PageCache& operator=(const PageCache&) = delete;

	static PageCache _sInst;
	std::recursive_mutex _pageMutex;	//一把大锁，一旦访问PageCache就要加锁

private:
	SpanList _spanlists[NPAGES];	//以页数为映射的规则(直接定址法)
	//std::unordered_map<PAGE_ID, Span*> _idSpanMap;
	//TCMalloc_PageMap1<BITSNUM> _idSpanMap;
	TCMalloc_PageMap3<BITSNUM> _idSpanMap;
	ObjectPool<Span> _spanPool;
};
