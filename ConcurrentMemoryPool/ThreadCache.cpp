#include "ThreadCache.h"
#include "CentralCache.h"

//从CentralCache获取内存
void* ThreadCache::FetchFromCentralCache(size_t index, size_t size)//size是索要的空间的大小(对齐后的字节数)
{
	//使用"慢开始"反馈调节算法计算从CentralCache到底索要多个对象
	//使用慢开始所达到的几个目的:
	//1. 保证最开始的一次不会向CentralCache索要太多,因为如果对方只用了一块该大小的内存后就再也不用了,此时会很浪费
	//2. 当你不断的要这个size大小的内存时，batchNum就会不断增长，直到上限
	//3. size越大时, 一次向CentralCache要的batchNum就会越小
	//4. size越小时，一次向CentralCache要的batchNum就会越大
	size_t batchNum = min(_freelists[index].MaxSize(), SizeClass::SizeToMaxBatchNum(size));
	if (_freelists[index].MaxSize() == batchNum)
	{
		_freelists[index].MaxSize() += 1;
	}

	void* start = nullptr;
	void* end = nullptr;
	size_t actualNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, size);

	if (actualNum == 1)
	{
		assert(start == end);
	}
	else
	{
		//因为CentralCache往往会给大于ThreadCache申请的内存，所以把本次不需要的内存块挂到ThreadCache的对应freelist上，需要的内存块就直接返回
		_freelists[index].PushRange(Next(start), end);
	}
	return start;
}

//向ThreadCache申请空间
void* ThreadCache::Allocate(size_t size)
{
	assert(size <= MAX_BYTES);
	size_t alignNum = SizeClass::RoundUp(size);	//对齐后的字节数(对齐到每块对象的大小为alignNum的自由链表上)
	size_t index = SizeClass::Index(size);		//具体是第几号自由链表(数组下标)

	if (!_freelists[index].Empty())
	{
		return _freelists[index].Pop();	//从freelist上对应的桶中分配一块空间
	}
	else
	{
		return FetchFromCentralCache(index, alignNum);	//从CentralCache上申请一块空间并返回
	}
}

void ThreadCache::Deallocate(void* ptr, size_t size)
{
	assert(ptr);
	assert(size <= MAX_BYTES);

	int index = SizeClass::Index(size);	//计算出应该插到哪个自由链表上
	_freelists[index].Push(ptr);	//将ptr这块空间头插到对应的自由链表上
}