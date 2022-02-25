#pragma once
#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include <mutex>


#include <time.h>
#include <assert.h>

using std::cout;
using std::endl;

static const size_t MAX_BYTES = 256 * 1024;	//小于等于MAX_BYTES就找ThreadCache
static const size_t NFREELISTS = 208;	//一个ThreadCache中自由链表的个数

#ifdef _WIN64	//64位下_WIN64和_WIN32都有，32位下只有_WIN32
	typedef unsigned long long PAGE_ID;
#elif _WIN32
	typedef size_t PAGE_ID;
#else
	//linux
#endif

//返回obj对象当中'用于存储下一个对象的地址'的引用
static inline void*& Next(void* obj)	//static要加上(其实有inline就没问题了)，不然会在多个.cpp里面重定义
{
	return *(void**)obj;
}

//管理切分好的定长对象的自由链表(每个ThreadCache里面有很多个FreeList)
class FreeList
{
public:
	void Push(void* obj)
	{
		//头插
		assert(obj);
		Next(obj) = _head;
		_head = obj;
	}

	void PushRange(void* start, void* end)
	{
		//也是头插: 先让end的下一个指向head指向的位置，然后再把head移动到start的位置(画个图)
		assert(start && end);
		Next(end) = _head;
		_head = start;
	}

	void* Pop()
	{
		//头删
		assert(_head);
		void* obj = _head;
		_head = Next(obj);
		return obj;
	}

	bool Empty()
	{
		return _head == nullptr;
	}

	size_t& MaxSize()
	{
		return _maxsize;
	}
private:
	void* _head = nullptr;
	size_t _maxsize = 1;
};


// 管理 空间范围划分与对齐、映射关系 的类
class SizeClass
{
public:
	// 整体控制在最多10%左右的内碎片浪费
	// [1,128]				8byte对齐			 freelist[0,16)
	// [128+1,1024] 1		6byte对齐			freelist[16,72)
	// [1024+1,8*1024]		128byte对齐			freelist[72,128)
	// [8*1024+1,64*1024]	1024byte对齐			freelist[128,184)
	// [64*1024+1,256*1024] 8*1024byte对齐		freelist[184,208)

	static inline size_t _RoundUp(size_t bytes, size_t alignNum)	//align对齐数
	{
		//易于理解的写法
		//if (bytes % alignNum != 0)
		//{
		//	return (bytes + alignNum) / alignNum * alignNum;
		//}
		//else
		//{
		//	return bytes;
		//}

		//更优秀的写法
		return ((bytes + alignNum - 1) & ~(alignNum - 1));
	}

	static inline size_t RoundUp(size_t bytes)
	{
		if (bytes <= 128)
		{
			return _RoundUp(bytes, 8);
		}
		else if (bytes <= 1024)
		{
			return _RoundUp(bytes, 16);
		}
		else if (bytes <= 8 * 1024)
		{
			return _RoundUp(bytes, 128);
		}
		else if (bytes <= 64 * 1024)
		{
			return _RoundUp(bytes, 1024);
		}
		else if (bytes <= 256 * 1024)
		{
			return _RoundUp(bytes, 8 * 1024);
		}
		else
		{
			assert(false);
			return -1;
		}
	}

	//易于理解的写法
	//static inline size_t _Index(size_t bytes, size_t align_shift)
	//{
	//	if (bytes % (1 << align_shift) == 0)
	//	{
	//		return bytes / (1 << align_shift) - 1;
	//	}
	//	else
	//	{
	//		return bytes / (1 << align_shift);
	//	}
	//}

	//bytes是字节数，align_shift是该字节数所遵守的对齐数(以位运算中需要左移的位数表示)
	static inline size_t _Index(size_t bytes, size_t align_shift)
	{
		return ((bytes + (1 << align_shift) - 1) >> align_shift) - 1;
	}

	static inline size_t Index(size_t bytes)
	{
		assert(bytes <= MAX_BYTES);

		//static int group_freelist[4] = { 16, 56, 56, 56 };
		static int group_freelist[4] = { 16, 72, 128, 184};	//用于扣除前面不属于当前对齐数的自由链表的个数
		if (bytes <= 128)
			return _Index(bytes, 3);	//3是2^3, 这里传的是 使用位运算要达到对齐数需要左移的位数
		else if (bytes <= 1024)
			return _Index(bytes - 128, 4) + group_freelist[0];
		else if (bytes <= 8 * 1024)
			return _Index(bytes - 8 * 1024, 7) + group_freelist[1];
		else if (bytes <= 64 * 1024)
			return _Index(bytes - 64 * 1024, 10) + group_freelist[2];
		else if (bytes <= 256 * 1024)
			return _Index(bytes - 256 * 1024, 13) + group_freelist[3];
		else
			assert(false);
		return -1;
	}

	// 一次ThreadCache应该向CentralCache申请的对象的个数(根据对象的大小计算)
	static size_t SizeToNum(size_t size)
	{
		assert(size > 0);

		// [2, 512],一次批量移动多少个对象的(慢启动)上限值
		// 小对象一次批量上限高
		// 小对象一次批量上限低
		int num = MAX_BYTES / size;
		if (num < 2)
			num = 2;
		if (num > 512)
			num = 512;
		return num;
	}
};


//一个Span是用来管理多个连续页的大块内存的
struct Span	//这个结构就类似于ListNode，因为它是构成SpanList的单个结点
{
	PAGE_ID _pageID;//大块内存起始页的页号(将一个进程的地址空间以页为单位划分，32下页的数量是2^32/2^13;64位下页的数量是2^64/2^13;假设一页是8K)
	size_t _n;		//页的数量
	size_t _useCount;//将切好的小块内存分给threadcache，useCount记录分出去了多少个小块内存

	Span* _next = nullptr;
	Span* _prev = nullptr;

	void* _freelist;//大块内存切成小块并连接起来，这样当threadcache要的时候直接给它小块的，回收的时候也方便管理
};

//带头的双向循环链表(将每个桶的位置处的多个Span连接起来)
class SpanList
{
public:
	SpanList()
	{
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}


	void Insert(Span* pos, Span* newSpan)
	{
		assert(pos && newSpan);
		Span* prev = pos->_prev;

		newSpan->_next = pos;
		newSpan->_prev = prev;
		pos->_prev = newSpan;
		prev->_next = newSpan;
	}

	void Erase(Span* pos)
	{
		assert(pos);
		Span* prev = pos->_prev;
		Span* next = pos->_next;

		prev->_next = next;
		next->_prev = prev;
	}

	void Lock() { _mtx.lock(); }
	void UnLock() { _mtx.unlock(); }
private:
	Span* _head = nullptr;
	std::mutex _mtx;	//桶锁: 进到桶里的时候才会加锁
};