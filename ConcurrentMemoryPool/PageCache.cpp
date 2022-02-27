#include "PageCache.h"

PageCache PageCache::_sInst;	//静态成员类外定义

//NewSpan分配Span有3种情况:(每次只给CentralCache 1个Span)
//1. CentralCache索要的页号下的Spanlist中存有Span，那么直接分配给它1个Span就可以了
//2. CentralCache索要的页号下的Spanlist中没有Span, 那么从当前位置往后遍历Spanlist[k + 1], 找到Span后，对Span进行切分，然后再挂载，最后分配给CentralCache
//3. 当前位置往后的Spanlist没有Span了，那么向系统(堆区)申请一块NPAGES大小的Span，然后对其进行切分和挂载，最后分配给CentralCache
Span* PageCache::NewSpan(size_t page)
{
	//this->PageLock();
	//先看当前页数的位置是否有Span
	if (!_spanlists[page].Empty())
	{
		Span* span = _spanlists[page].PopFront();
		//this->PageUnLock();	//这里是在已知道只会递归1次的情况下才选择手动UnLock()的，不然推荐使用unique_lock
		//this->PageUnLock();	//正常情况就是, 递归加锁几次，就要解锁几次
		return span;
	}

	//再去查看当前位置以后的位置是否有Span
	for (int i = page + 1; i < NPAGES; ++i)
	{
		//找到了Span
		if (!_spanlists[i].Empty())
		{
			Span* BigSpan = _spanlists[i].PopFront();
			Span* SmallSpan = new Span;	//new一个小的span用于存放其中一个切分好的span

			SmallSpan->_n = page;
			SmallSpan->_pageID = BigSpan->_pageID;
			BigSpan->_n -= page;
			//BigSpan->_pageID += (page << PAGE_SHIFT);	//错误2
			BigSpan->_pageID += page;					//修改2

			//也要把BigSpan的首页和尾页存到映射表当中，这样在空闲的Span进行合并查找时能够通过映射表找到BigSpan随后进行合并
			_idSpanMap[BigSpan->_pageID] = BigSpan;
			_idSpanMap[BigSpan->_pageID + BigSpan->_n - 1] = BigSpan;	//别忘了-1,举个例子看看

			_spanlists[BigSpan->_n].PushFront(BigSpan);
			//this->PageUnLock();
			//this->PageUnLock();

			//保存返回给CentralCache时的SmallSpan的每一页与SmallSpan的映射关系, 这样方便SmallSpan切分成小块内存后，然后小块内存通过映射关系找回SmallSpan
			for (size_t i = SmallSpan->_pageID; i < SmallSpan->_pageID + SmallSpan->_n; ++i)
			{
				_idSpanMap[i] = SmallSpan;
			}
			return SmallSpan;
		}
	}

	//错误1
	//当前位置以后的位置也没有Span,那么直接向堆申请一块大小为(NPAGES-1)<<PAGE_SHIFT的空间
	//Span* newSpan = (Span*)SystemAlloc(NPAGES - 1);	//这里传过去的是页数
	//newSpan->_n = NPAGES - 1;
	//newSpan->_pageID = ((int)newSpan >> PAGE_SHIFT);
	//_spanlists[NPAGES - 1].PushFront(newSpan);
	//return NewSpan(page);	//递归调用自己，复用之前的代码(当然也可以再创建一个子函数，让子函数执行)

	//修改1
	Span* newSpan = new Span;	//Span对象是new出来的
	void* ptr = SystemAlloc(NPAGES - 1);	//向系统申请的空间是交给newSpan中的_pageID和_n进行管理的
	newSpan->_n = NPAGES - 1;
	newSpan->_pageID = ((PAGE_ID)ptr >> PAGE_SHIFT);
	_spanlists[NPAGES - 1].PushFront(newSpan);
	return NewSpan(page);
}

//将PAGE_ID映射到一个Span*上, 这样可以通过页号直接找到对应的Span*的位置
Span* PageCache::MapPAGEIDToSpan(PAGE_ID id)
{
	auto it = _idSpanMap.find(id);
	if (it != _idSpanMap.end())
	{
		return it->second;
	}
	return nullptr;	//没找到映射关系, 返回nullptr
}

//将useCount减为0的Span返回给PageCache，以用来合并成更大的Span
void PageCache::ReleaseSpanToPageCache(Span* span)
{
	//先向前+向后查找，看是否有空闲的Span能进行合并，当不能再合并的时候，再把span插入到对应位置
	//1. 向前查找
	while (1)
	{
		PAGE_ID prevID = span->_pageID - 1;	//计算span的前一个Span的最后一页的页号
		Span* prevSpan = MapPAGEIDToSpan(prevID);//通过映射关系，找到前面一个页所在的Span当中
		if (prevSpan == nullptr)	//判断前面的Span是否被申请过(可能还在系统中)[如果在映射表当中，就证明被申请过]
			break;
		if (prevSpan->_isUsed == true)	//前面的Span正在被CentralCache使用(这里不能使用orevSpan的useCount作为判断依据，因为useCount的值的变化存在间隙[在切分Span时])
			break;
		if (prevSpan->_n + span->_n > NPAGES - 1)//合并后的大小>128时, 不能合并
			break;
		//开始合并
		span->_pageID = prevSpan->_pageID;
		span->_n += prevSpan->_n;

		_spanlists[prevSpan->_n].Erase(prevSpan);
		delete prevSpan;	//prevSpan对象直接释放
		prevSpan = nullptr;
	}
	//2. 向后查找
	while (1)
	{
		PAGE_ID nextID = span->_pageID + span->_n;
		Span* nextSpan = MapPAGEIDToSpan(nextID);
		if (nextSpan == nullptr)
			break;
		if (nextSpan->_isUsed == true)
			break;
		if (nextSpan->_n + span->_n > NPAGES - 1)
			break;
		//开始合并
		span->_n += nextSpan->_n;

		_spanlists[nextSpan->_n].Erase(nextSpan);
		delete nextSpan;
		nextSpan = nullptr;
	}

	span->_isUsed = false;
	_spanlists[span->_n].PushFront(span);	//将合并后的Span插入到spanlist当中
	_idSpanMap[span->_pageID] = span;
	_idSpanMap[span->_pageID + span->_n - 1] = span;
}