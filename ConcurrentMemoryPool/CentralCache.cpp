#include "CentralCache.h"

//获取一个非空的Span
Span* CentralCache::GetOneSpan(SpanList& list, size_t size)
{
	//...
	return nullptr;
}

//从CentralCache获取一定数量(batchNum)的对象给ThreadCache。输出型参数:start、end
//该函数的返回值是actualNum:实际上获取到的对象数量
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size)
{
	size_t index = SizeClass::Index(size);	//确定哪个桶
	_spanlists[index].Lock();	//进到index这个桶了，要先加锁

	Span* span = GetOneSpan(_spanlists[index], size);
	start = span->_freelist;
	end = start;
	size_t actualNum = 1;
	while (--batchNum && Next(end) != nullptr)	//--batchNum就是让end向后移动batchNum-1步
	{
		++actualNum;
		end = Next(end);
	}
	span->_freelist = Next(end);
	Next(end) = nullptr;	//获取完start和end这段范围的空间了，得让end与之后的空间断开连接

	_spanlists[index].UnLock();
	return actualNum;
}