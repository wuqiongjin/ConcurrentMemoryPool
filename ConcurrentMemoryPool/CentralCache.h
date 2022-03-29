#pragma once
#include "Common.h"

//由于全局只能有一个CentralCache对象，所以这里设计为单例模式
class CentralCache
{
public:
	static CentralCache* GetInstance()
	{
    return &_sInst;
    //static std::mutex mtx;
    //if(_sInst == nullptr)
    //{
    //  mtx.lock();
    //  if(_sInst == nullptr)
    //  {
    //    _sInst = new CentralCache;
    //  }
    //  mtx.unlock();
    //}
    //return _sInst;
	}
	
	//获取一个非空的Span
	Span* GetOneSpan(SpanList& list, size_t size);

	//从CentralCache获取一定数量的对象给ThreadCache
	size_t FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size);

	//将从start起始的freelist中的所有小块内存都归还给spanlists[index]中的各自的span
	void ReleaseListToSpans(void* start, size_t size);
private:
	CentralCache(){}
	CentralCache(const CentralCache&) = delete;
	CentralCache& operator=(const CentralCache&) = delete;
	
	static CentralCache _sInst;	//声明(定义在.cpp里面)
private:
	SpanList _spanlists[NFREELISTS];	//哈希桶结构
};
