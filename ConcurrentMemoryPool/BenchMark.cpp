#include "ConcurrentAllocate.h"
#include <atomic>

// ntimes 一轮申请和释放内存的次数
// rounds 轮次
void BenchmarkMalloc(size_t ntimes, size_t nworks, size_t rounds)
{
	std::vector<std::thread> vthread(nworks);
	std::atomic<unsigned int> malloc_costtime;
  malloc_costtime = 0;
	std::atomic<unsigned int> free_costtime;
  free_costtime = 0;

	for (size_t k = 0; k < nworks; ++k)
	{
		vthread[k] = std::thread([&]() {
			std::vector<void*> v;
			v.reserve(ntimes);

			for (size_t j = 0; j < rounds; ++j)
			{
				size_t begin1 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					//v.push_back(malloc(16));
					v.push_back(malloc((16 + i) % 8192 + 1));
				}
				size_t end1 = clock();

				size_t begin2 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					free(v[i]);
				}
				size_t end2 = clock();
				v.clear();

				malloc_costtime += (end1 - begin1);
				free_costtime += (end2 - begin2);
			}
		});
	}

	for (auto& t : vthread)
	{
		t.join();
	}

	//printf("%u个线程并发执行%u轮次，每轮次malloc %u次: 花费：%u ms\n",
	//	nworks, rounds, ntimes, malloc_costtime);

	cout << nworks << "个线程并发执行" << rounds << "轮次" << ",每轮次malloc" << ntimes << "次: 花费：" << malloc_costtime << "ms" << endl;

	//printf("%u个线程并发执行%u轮次，每轮次free %u次: 花费：%u ms\n",
	//	nworks, rounds, ntimes, free_costtime);

	cout << nworks << "个线程并发执行" << rounds << "轮次" << ",每轮次free" << ntimes << "次: 花费：" << free_costtime << "ms" << endl;

	//printf("%u个线程并发malloc&free %u次，总计花费：%u ms\n",
	//	nworks, nworks*rounds*ntimes, malloc_costtime + free_costtime);

	cout << nworks << "个线程并发执行malloc&free " << nworks*rounds*ntimes <<"次, 总计花费:：" << malloc_costtime + free_costtime << "ms" << endl;
}




// 单轮次申请释放次数 线程数 轮次
void BenchmarkConcurrentMalloc(size_t ntimes, size_t nworks, size_t rounds)
{
	std::vector<std::thread> vthread(nworks);
	std::atomic<unsigned long> malloc_costtime;
  malloc_costtime = 0;
	std::atomic<unsigned long> free_costtime;
  free_costtime = 0;

	for (size_t k = 0; k < nworks; ++k)
	{
		vthread[k] = std::thread([&]() {
			std::vector<void*> v;
			v.reserve(ntimes);

			for (size_t j = 0; j < rounds; ++j)
			{
				size_t begin1 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					//v.push_back(ConcurrentAllocate(16));
					v.push_back(ConcurrentAllocate((16 + i) % 8192 + 1));
				}
				size_t end1 = clock();

				size_t begin2 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					ConcurrentFree(v[i]);
				}
				size_t end2 = clock();
				v.clear();

				malloc_costtime += (end1 - begin1);
				free_costtime += (end2 - begin2);
			}

			//线程结束之前, Delete并调用ThreadCache的析构函数，释放ThreadCache中的小块对象返回给CentralCache
			if (pTLSThreadCache)
				tcPool.Delete(pTLSThreadCache);
		});
	}

	for (auto& t : vthread)
	{
		t.join();
	}

	//printf("%u个线程并发执行%u轮次，每轮次concurrent alloc %u次: 花费：%u ms\n",
	//	nworks, rounds, ntimes, malloc_costtime);

	//printf("%u个线程并发执行%u轮次，每轮次concurrent dealloc %u次: 花费：%u ms\n",
	//	nworks, rounds, ntimes, free_costtime);

	//printf("%u个线程并发concurrent alloc&dealloc %u次，总计花费：%u ms\n",
	//	nworks, nworks*rounds*ntimes, malloc_costtime + free_costtime);

	//printf("%u个线程并发执行%u轮次，每轮次malloc %u次: 花费：%u ms\n",
//	nworks, rounds, ntimes, malloc_costtime);

	cout << nworks << "个线程并发执行" << rounds << "轮次" << ",每轮次concurrent alloc" << ntimes << "次: 花费：" << malloc_costtime << "ms" << endl;

	//printf("%u个线程并发执行%u轮次，每轮次free %u次: 花费：%u ms\n",
	//	nworks, rounds, ntimes, free_costtime);

	cout << nworks << "个线程并发执行" << rounds << "轮次" << ",每轮次concurrent dealloc" << ntimes << "次: 花费：" << free_costtime << "ms" << endl;

	//printf("%u个线程并发malloc&free %u次，总计花费：%u ms\n",
	//	nworks, nworks*rounds*ntimes, malloc_costtime + free_costtime);

	cout << nworks << "个线程并发执行concurrent alloc&dealloc " << nworks * rounds*ntimes << "次, 总计花费:：" << malloc_costtime + free_costtime << "ms" << endl;
}

void Routine1()
{
	std::vector<void*> v1;
	for (int i = 0; i < 4; ++i)
	{
		void* ptr = ConcurrentAllocate(i + 1);
		v1.push_back(ptr);
	}

	for (int i = 0; i < 4; ++i)
	{
		ConcurrentFree(v1[i]);
	}

	if (pTLSThreadCache)
		tcPool.Delete(pTLSThreadCache);
}

void Routine2()
{
	std::vector<void*> v1;
	for (int i = 0; i < 4; ++i)
	{
		void* ptr = ConcurrentAllocate(i + 1);
		v1.push_back(ptr);
	}

	for (int i = 0; i < 4; ++i)
	{
		ConcurrentFree(v1[i]);
	}

	if (pTLSThreadCache)
		tcPool.Delete(pTLSThreadCache);
}

int main()
{
	size_t n = 1000;
	cout << "==========================================================" << endl;
	BenchmarkConcurrentMalloc(n, 4, 100);
	cout << endl << endl;

	BenchmarkMalloc(n, 4, 100);
	cout << "==========================================================" << endl;
	
	//std::thread t1(Routine1);
	//std::thread t2(Routine2);

	//t1.join();
	//t2.join();
	return 0;
}
