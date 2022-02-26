#include "PageCache.h"

PageCache PageCache::_sInst;	//静态成员类外定义

//NewSpan分配Span有3种情况:(每次只给CentralCache 1个Span)
//1. CentralCache索要的页号下的Spanlist中存有Span，那么直接分配给它1个Span就可以了
//2. CentralCache索要的页号下的Spanlist中没有Span, 那么从当前位置往后遍历Spanlist[k + 1], 找到Span后，对Span进行切分，然后再挂载，最后分配给CentralCache
//3. 当前位置往后的Spanlist没有Span了，那么向系统(堆区)申请一块NPAGES大小的Span，然后对其进行切分和挂载，最后分配给CentralCache
Span* PageCache::NewSpan(size_t page)
{
	this->PageLock();
	//先看当前页数的位置是否有Span
	if (!_spanlists[page].Empty())
	{
		Span* span = _spanlists[page].PopFront();
		this->PageUnLock();
		this->PageUnLock();
		return span;
	}

	//再去查看当前位置以后的位置是否有Span
	for (int i = page + 1; i < NPAGES; ++i)
	{
		//找到了Span
		if (!_spanlists[i].Empty())
		{
			Span* BigSpan = _spanlists[i].PopFront();
			Span* smallSpan = new Span;	//new一个小的span用于存放其中一个切分好的span

			smallSpan->_n = page;
			smallSpan->_pageID = BigSpan->_pageID;
			BigSpan->_n -= page;
			BigSpan->_pageID += (page << PAGE_SHIFT);

			_spanlists[BigSpan->_n].PushFront(BigSpan);
			this->PageUnLock();
			this->PageUnLock();
			return smallSpan;
		}
	}

	//当前位置以后的位置也没有Span,那么直接向堆申请一块大小为(NPAGES-1)<<PAGE_SHIFT的空间
	Span* newSpan = (Span*)SystemAlloc(NPAGES - 1);	//这里传过去的是页数
	newSpan->_n = NPAGES - 1;
	newSpan->_pageID = ((int)newSpan >> PAGE_SHIFT);
	_spanlists[NPAGES - 1].PushFront(newSpan);
	return NewSpan(page);	//递归调用自己，复用之前的代码(当然也可以再创建一个子函数，让子函数执行)
}
