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

static const size_t MAX_BYTES = 256 * 1024;	//С�ڵ���MAX_BYTES����ThreadCache
static const size_t NFREELISTS = 208;	//һ��ThreadCache����������ĸ���

#ifdef _WIN64	//64λ��_WIN64��_WIN32���У�32λ��ֻ��_WIN32
	typedef unsigned long long PAGE_ID;
#elif _WIN32
	typedef size_t PAGE_ID;
#else
	//linux
#endif

//����obj������'���ڴ洢��һ������ĵ�ַ'������
static inline void*& Next(void* obj)	//staticҪ����(��ʵ��inline��û������)����Ȼ���ڶ��.cpp�����ض���
{
	return *(void**)obj;
}

//�����зֺõĶ����������������(ÿ��ThreadCache�����кܶ��FreeList)
class FreeList
{
public:
	void Push(void* obj)
	{
		//ͷ��
		assert(obj);
		Next(obj) = _head;
		_head = obj;
	}

	void PushRange(void* start, void* end)
	{
		//Ҳ��ͷ��: ����end����һ��ָ��headָ���λ�ã�Ȼ���ٰ�head�ƶ���start��λ��(����ͼ)
		assert(start && end);
		Next(end) = _head;
		_head = start;
	}

	void* Pop()
	{
		//ͷɾ
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


// ���� �ռ䷶Χ��������롢ӳ���ϵ ����
class SizeClass
{
public:
	// ������������10%���ҵ�����Ƭ�˷�
	// [1,128]				8byte����			 freelist[0,16)
	// [128+1,1024] 1		6byte����			freelist[16,72)
	// [1024+1,8*1024]		128byte����			freelist[72,128)
	// [8*1024+1,64*1024]	1024byte����			freelist[128,184)
	// [64*1024+1,256*1024] 8*1024byte����		freelist[184,208)

	static inline size_t _RoundUp(size_t bytes, size_t alignNum)	//align������
	{
		//��������д��
		//if (bytes % alignNum != 0)
		//{
		//	return (bytes + alignNum) / alignNum * alignNum;
		//}
		//else
		//{
		//	return bytes;
		//}

		//�������д��
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

	//��������д��
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

	//bytes���ֽ�����align_shift�Ǹ��ֽ��������صĶ�����(��λ��������Ҫ���Ƶ�λ����ʾ)
	static inline size_t _Index(size_t bytes, size_t align_shift)
	{
		return ((bytes + (1 << align_shift) - 1) >> align_shift) - 1;
	}

	static inline size_t Index(size_t bytes)
	{
		assert(bytes <= MAX_BYTES);

		//static int group_freelist[4] = { 16, 56, 56, 56 };
		static int group_freelist[4] = { 16, 72, 128, 184};	//���ڿ۳�ǰ�治���ڵ�ǰ����������������ĸ���
		if (bytes <= 128)
			return _Index(bytes, 3);	//3��2^3, ���ﴫ���� ʹ��λ����Ҫ�ﵽ��������Ҫ���Ƶ�λ��
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

	// һ��ThreadCacheӦ����CentralCache����Ķ���ĸ���(���ݶ���Ĵ�С����)
	static size_t SizeToNum(size_t size)
	{
		assert(size > 0);

		// [2, 512],һ�������ƶ����ٸ������(������)����ֵ
		// С����һ���������޸�
		// С����һ���������޵�
		int num = MAX_BYTES / size;
		if (num < 2)
			num = 2;
		if (num > 512)
			num = 512;
		return num;
	}
};


//һ��Span����������������ҳ�Ĵ���ڴ��
struct Span	//����ṹ��������ListNode����Ϊ���ǹ���SpanList�ĵ������
{
	PAGE_ID _pageID;//����ڴ���ʼҳ��ҳ��(��һ�����̵ĵ�ַ�ռ���ҳΪ��λ���֣�32��ҳ��������2^32/2^13;64λ��ҳ��������2^64/2^13;����һҳ��8K)
	size_t _n;		//ҳ������
	size_t _useCount;//���кõ�С���ڴ�ָ�threadcache��useCount��¼�ֳ�ȥ�˶��ٸ�С���ڴ�

	Span* _next = nullptr;
	Span* _prev = nullptr;

	void* _freelist;//����ڴ��г�С�鲢����������������threadcacheҪ��ʱ��ֱ�Ӹ���С��ģ����յ�ʱ��Ҳ�������
};

//��ͷ��˫��ѭ������(��ÿ��Ͱ��λ�ô��Ķ��Span��������)
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
	std::mutex _mtx;	//Ͱ��: ����Ͱ���ʱ��Ż����
};