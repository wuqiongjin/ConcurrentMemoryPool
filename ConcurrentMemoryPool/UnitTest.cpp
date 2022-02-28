//#include "ObjectPool.h"
#include "ConcurrentAllocate.h"

void TLSAlloc1()
{
	for (size_t i = 0; i < 5; ++i)
	{
		void* ptr = ConcurrentAllocate(6);
	}
}

void TLSAlloc2()
{
	for (size_t i = 0; i < 5; ++i)
	{
		void* ptr = ConcurrentAllocate(6);
	}
}


void testTLS()
{
	std::thread t1(TLSAlloc1);
	t1.join();

	std::thread t2(TLSAlloc2);
	t2.join();
}

void testConcurrentAlloc()
{
	void* ptr1 = ConcurrentAllocate(6);
	void* ptr2 = ConcurrentAllocate(7);
	void* ptr3 = ConcurrentAllocate(8);
	void* ptr4 = ConcurrentAllocate(5);
	void* ptr5 = ConcurrentAllocate(3);
	void* ptr6 = ConcurrentAllocate(7);
	void* ptr7 = ConcurrentAllocate(8);
	void* ptr8 = ConcurrentAllocate(5);

	ConcurrentFree(ptr1);
	ConcurrentFree(ptr2);
	ConcurrentFree(ptr3);
	ConcurrentFree(ptr4);
	ConcurrentFree(ptr5);
	ConcurrentFree(ptr6);
	ConcurrentFree(ptr7);
	ConcurrentFree(ptr8);
}


void MultiAlloc1()
{
	std::vector<void*> v;
	for (int i = 0; i < 10; ++i)
	{
		void* ptr = ConcurrentAllocate(6);
		v.push_back(ptr);
	}

	for (int i = 0; i < 10; ++i)
	{
		ConcurrentFree(v[i]);
	}
}

void MultiAlloc2()
{
	std::vector<void*> v;
	for (int i = 0; i < 10; ++i)
	{
		void* ptr = ConcurrentAllocate(7);
		v.push_back(ptr);
	}

	for (int i = 0; i < 10; ++i)
	{
		ConcurrentFree(v[i]);
	}
}

void MultiAlloc3()
{
	std::vector<void*> v;
	for (int i = 0; i < 10; ++i)
	{
		void* ptr = ConcurrentAllocate(7);
		v.push_back(ptr);
	}

	for (int i = 0; i < 10; ++i)
	{
		ConcurrentFree(v[i]);
	}
}

void TestMultiThread()
{
	std::thread t1(MultiAlloc1);
	std::thread t2(MultiAlloc2);
	std::thread t3(MultiAlloc3);

	t1.join();
	t2.join();
	t3.join();
}


void TestBigAlloc()
{
	void* ptr1 = ConcurrentAllocate(257 * 1024);
	void* ptr2 = ConcurrentAllocate(129 << PAGE_SHIFT);

	ConcurrentFree(ptr1);
	ConcurrentFree(ptr2);

	//ConcurrentFree(ptr1, 257 * 1024);
	//ConcurrentFree(ptr2, 129 << PAGE_SHIFT);
}


int main()
{
	//TestObjectPool();
	//testTLS();
	//testConcurrentAlloc();

	
	TestMultiThread();
	//TestBigAlloc();


	return 0;
}