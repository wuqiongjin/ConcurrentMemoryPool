#include "CentralCache.h"
#include "PageCache.h"

//static成员需要在类外定义和初始化
CentralCache CentralCache::_sInst;

//获取一个非空的Span
Span* CentralCache::GetOneSpan(SpanList& slist, size_t size)
{
	//遍历整个Span找非空的span
	Span* it = slist.Begin();
	while (it != slist.End())
	{
		if (it->_freelist != nullptr)
		{
			return it;	//找到了非空的span，直接返回
		}
		it = it->_next;
	}

	//在向PageCache索要之前，可以先解锁，让其它线程能够进入当前的桶中(可能有线程把内存释放回来)
	slist.UnLock();
	//没找到非空的span, 向PageCache索要
	Span* newSpan = PageCache::GetInstance()->NewSpan(SizeClass::SizeToPage(size));	//newSpan是局部变量，其它线程看不到，所以对newSpan的操作不需要加锁

	//将newSpan切分成小块内存, 并将这些小块内存放到newSpan的成员_freelist下，使用Next(obj)函数进行连接
	//先计算出大块内存的起始地址(页号*每页的大小)和终止地址(页数*每页的大小+起始地址)
	char* start = (char*)(newSpan->_pageID << PAGE_SHIFT);
	char* end = start + (newSpan->_n << PAGE_SHIFT);
	
	//_freelist下最好搞一个头结点，方便头插
	newSpan->_freelist = start;

	//开始切分
	while (start < end)
	{
		Next(start) = start + size;//每块小内存填写下一块小内存的地址以达成逻辑上连接的效果
		start += size;
	}

	//PushFront的操作涉及到临界区，因此需要加锁，解锁的操作在FetchRangeObj中(因为进入GetOneSpan函数时是带着桶锁进来的!)
	slist.Lock();
	slist.PushFront(newSpan);	//将新的newSpan插入到slist中
	return newSpan;
}

//从CentralCache获取一定数量(batchNum)的对象给ThreadCache。输出型参数:start、end
//该函数的返回值是actualNum:实际上获取到的对象数量
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size)
{
	size_t index = SizeClass::Index(size);	//确定哪个桶
	_spanlists[index].Lock();	//进到index这个桶了，要先加锁

	Span* span = GetOneSpan(_spanlists[index], size);
	assert(span && span->_freelist);

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