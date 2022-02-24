#include "ThreadCache.h"

void* ThreadCache::FetchFromCentralCache(size_t Index, size_t alignNum)
{
	//...
	return nullptr;
}

//向ThreadCache申请空间
void* ThreadCache::Allocate(size_t size)
{
	assert(size <= MAX_BYTES);
	size_t alignNum = SizeClass::RoundUp(size);	//对齐数(对齐到每块对象的大小为alignNum的自由链表上)
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