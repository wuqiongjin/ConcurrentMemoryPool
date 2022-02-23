#pragma once
#include <iostream>
#include <vector>
#include <time.h>
using std::cout;
using std::endl;

#ifdef _WIN32
#include <Windows.h>
#else
//
#endif

inline static void* SystemAlloc(size_t bytes)
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, bytes, MEM_COMMIT | MEM_RESERVE,	//直接向系统申请空间
		PAGE_READWRITE);
#else
	// linux下brk mmap等
#endif
	if (ptr == nullptr)
		throw std::bad_alloc();
	return ptr;
}

template <class T>
class ObjectPool
{
public:
	T* New()
	{
		T* obj;
		if (_freeList)	//freeList里面存有空闲空间时，优先从freeList里面取
		{
			obj = (T*)_freeList;
			_freeList = *(void**)_freeList;	//将free	List跳转到下一个对象处
		}
		else
		{
			if (_remainBytes < sizeof(T))
			{
				_remainBytes = 128 * 1024;	//如果_remainBytes<sizeof(T)并且不为0时，扔掉这部分空间，重新申请
				_memory = (char*)SystemAlloc(_remainBytes);
				if (_memory == nullptr)
				{
					throw std::bad_alloc();
				}
			}

			obj = (T*)_memory;
			//如果对象的大小小于指针的大小，那么我们也要每次给对象分配最低指针的大小。因为freeList必须能存得下一个指针的大小
			size_t objSize = sizeof(T) > sizeof(void*) ? sizeof(T) : sizeof(void*);
			_memory += objSize;
			_remainBytes -= objSize;
		}
		
		//使用定位new调用obj对象的构造函数
		new(obj)T;
		return obj;
	}

	void Delete(T* ptr)
	{
		ptr->~T();	//手动调用T对象的析构函数
		//头插法，将不用的空间插到freeList当中
		*(void**)ptr = _freeList;	//在32位下，*(void**)能看到4个字节；在64位下，*(void**)能看到8个字节
		_freeList = ptr;
	}

private:
	char* _memory = nullptr;	//指向内存块的指针
	size_t _remainBytes = 0;	//内存块的剩余空间字节数
	void* _freeList = nullptr;	//管理从内存块中所释放出来的空间的自由链表
};

//测试
struct TreeNode
{
	int _val;
	TreeNode* _left;
	TreeNode* _right;
	TreeNode()
		:_val(0)
		, _left(nullptr)
		, _right(nullptr)
	{}
};
void TestObjectPool()
{
	// 申请释放的轮次
	const size_t Rounds = 3;
	// 每轮申请释放多少次
	const size_t N = 100000;
	std::vector<TreeNode*> v1;
	v1.reserve(N);
	size_t begin1 = clock();
	for (size_t j = 0; j < Rounds; ++j)
	{
		for (int i = 0; i < N; ++i)
		{
			v1.push_back(new TreeNode);
		}
		for (int i = 0; i < N; ++i)
		{
			delete v1[i];
		}
		v1.clear();
	}
	size_t end1 = clock();

	ObjectPool<TreeNode> TNPool;
	std::vector<TreeNode*> v2;
	v2.reserve(N);
	size_t begin2 = clock();
	for (size_t j = 0; j < Rounds; ++j)
	{
		for (int i = 0; i < N; ++i)
		{
			v2.push_back(TNPool.New());
		}
		for (int i = 0; i < N; ++i)
		{
			TNPool.Delete(v2[i]);
		}
		v2.clear();
	}
	size_t end2 = clock();
	cout << "new cost time:" << end1 - begin1 << endl;
	cout << "object pool cost time:" << end2 - begin2 << endl;
}