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
	PageCache::GetInstance()->PageLock();	//在这加锁也没问题, 如果再NewSpan函数里加锁, 就需要使用递归锁的特性了
	Span* newSpan = PageCache::GetInstance()->NewSpan(SizeClass::SizeToPage(size));	//newSpan是局部变量，其它线程看不到，所以对newSpan的操作不需要加锁
	//newSpan->_isUsed = true;	//newSpan是即将分配给CentralCache使用的Span	//修改: 考虑到调用NewSpan的函数不止这一个, isUsed状态最好在NewSpan里面设置
	PageCache::GetInstance()->PageUnLock();

	newSpan->_ObjectSize = size;

	//将newSpan切分成小块内存, 并将这些小块内存放到newSpan的成员_freelist下，使用Next(obj)函数进行连接
	//先计算出大块内存的起始地址(页号*每页的大小)和终止地址(页数*每页的大小+起始地址)
	char* start = (char*)(newSpan->_pageID << PAGE_SHIFT);
	char* end = start + (newSpan->_n << PAGE_SHIFT);

	//_freelist下最好搞一个头结点，方便尾插
	newSpan->_freelist = (void*)start;
	void* ret = start;
	start += size;
	Next(ret) = start;

	int j = 1;
	//开始切分
	while (start < end)
	{
		++j;
		//Next(start) = (void*)(start + size);//每块小内存填写下一块小内存的地址以达成逻辑上连接的效果
		////start = (char*)Next(start);
		//start += size;	//等价于上面的
		Next(ret) = start;
		ret = Next(ret);
		start += size;
	}
	//忘记在切分完对象后让最后一小块对象的Next设置为nullptr了
	Next(ret) = nullptr;
	
	//条件断点 + 全部中断		-----调试-----
	//void* cur = newSpan->_freelist;
	//int i = 0;
	//while (cur)
	//{
	//	++i;
	//	cur = Next(cur);
	//}
	//if (i != ((newSpan->_n << PAGE_SHIFT) / size))
	//{
	//	cout << "error" << endl;
	//}
	//if (newSpan->_freelist != (void*)(newSpan->_pageID << PAGE_SHIFT))
	//{
	//	assert(false);
	//}

	//cout << std::this_thread::get_id() << ":" << " start:" << (void*)start << endl;
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

	//void* cur = start;
	//int i = 0;
	//while (cur)
	//{
	//	cur = Next(cur);
	//	++i;
	//}
	//if (i != actualNum)
	//{
	//	cout << "error!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
	//	system("pause");
	//}

	span->_useCount += actualNum;

	_spanlists[index].UnLock();
	return actualNum;
}

//将从start起始的freelist中的所有小块内存都归还(头插)给spanlists[index]中的各自的span
void CentralCache::ReleaseListToSpans(void* start, size_t size)
{
	assert(start);
	size_t index = SizeClass::Index(size);
	_spanlists[index].Lock();

	//int i = 0;
	//void* cur = start;
	//while (cur)
	//{
	//	cur = Next(cur);
	//	++i;
	//}

	while (start)
	{
		void* NextPos = Next(start);
		PAGE_ID id = ((PAGE_ID)start >> PAGE_SHIFT);//在某一页当中的所有地址除以页的大小，它们得到的结果都是当前页的页号
		//PageCache::GetInstance()->PageLock();	//基数树优化
		Span* ret = PageCache::GetInstance()->MapPAGEIDToSpan(id);//映射关系在PageCache将Span分给CentralCache时进行存储
		//PageCache::GetInstance()->PageUnLock();
		//将start小块内存头插到Span* ret的_freelist当中
		if(ret != nullptr)
		{
			Next(start) = ret->_freelist;
			ret->_freelist = start;
		}
		else
		{
			assert(false);	//不应该是nullptr
		}

		//将useCount减为0的Span返回给PageCache，以用来合并成更大的Span

		//-----调试-----
		//--ret->_useCount;
		//if (ret->_useCount == 4294967295)
		//{
		//	int x = 0;
		//	if (ret->_freelist == nullptr && ret->_prev == nullptr && ret->_next == nullptr)
		//	{
		//		int y = 0;
		//	}
		//}

		if (ret->_useCount == 0)
		{
			//错误3，修改3:--->忘记把useCount == 0的span从CentralCache扔掉了
			_spanlists[index].Erase(ret);
			ret->_freelist = nullptr;
			ret->_prev = nullptr;
			ret->_next = nullptr;

			_spanlists[index].UnLock();
			PageCache::GetInstance()->PageLock();
			PageCache::GetInstance()->ReleaseSpanToPageCache(ret);
			PageCache::GetInstance()->PageUnLock();
			_spanlists[index].Lock();
		}

		start = NextPos;
	}
	_spanlists[index].UnLock();
}
