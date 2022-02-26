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
}

int main()
{
	//TestObjectPool();
	//testTLS();
	testConcurrentAlloc();
	return 0;
}