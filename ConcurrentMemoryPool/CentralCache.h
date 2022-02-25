#pragma once
#include "Common.h"

//由于全局只能有一个CentralCache对象，所以这里设计为单例模式
class CentralCache
{
public:
	static CentralCache* GetInstance()
	{
		return &_sInst;
	}
	
	//获取一个非空的Span
	Span* GetOneSpan(SpanList& list, size_t size);

	//从CentralCache获取一定数量的对象给ThreadCache
	size_t FetchRangeObj(void*& start, void*&end, size_t batchNum, size_t size);
private:
	CentralCache(){}
	CentralCache(const CentralCache&) = delete;
	CentralCache& operator=(const CentralCache&) = delete;
	
	static CentralCache _sInst;	//声明
private:
	SpanList _spanlists[NFREELISTS];	//哈希桶结构
};
CentralCache CentralCache::_sInst;	//static成员需要在类外定义和初始化