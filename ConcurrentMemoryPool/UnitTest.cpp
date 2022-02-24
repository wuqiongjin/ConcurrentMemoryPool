#include "ObjectPool.h"
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

int main()
{
	//TestObjectPool();
	testTLS();
	return 0;
}