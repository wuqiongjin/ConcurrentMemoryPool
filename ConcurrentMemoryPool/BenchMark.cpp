#include "ConcurrentAllocate.h"
#include <atomic>

// ntimes һ��������ͷ��ڴ�Ĵ���
// rounds �ִ�
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

	//printf("%u���̲߳���ִ��%u�ִΣ�ÿ�ִ�malloc %u��: ���ѣ�%u ms\n",
	//	nworks, rounds, ntimes, malloc_costtime);

	cout << nworks << "���̲߳���ִ��" << rounds << "�ִ�" << ",ÿ�ִ�malloc" << ntimes << "��: ���ѣ�" << malloc_costtime << "ms" << endl;

	//printf("%u���̲߳���ִ��%u�ִΣ�ÿ�ִ�free %u��: ���ѣ�%u ms\n",
	//	nworks, rounds, ntimes, free_costtime);

	cout << nworks << "���̲߳���ִ��" << rounds << "�ִ�" << ",ÿ�ִ�free" << ntimes << "��: ���ѣ�" << free_costtime << "ms" << endl;

	//printf("%u���̲߳���malloc&free %u�Σ��ܼƻ��ѣ�%u ms\n",
	//	nworks, nworks*rounds*ntimes, malloc_costtime + free_costtime);

	cout << nworks << "���̲߳���ִ��malloc&free " << nworks*rounds*ntimes <<"��, �ܼƻ���:��" << malloc_costtime + free_costtime << "ms" << endl;
}




// ���ִ������ͷŴ��� �߳��� �ִ�
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

			//�߳̽���֮ǰ, Delete������ThreadCache�������������ͷ�ThreadCache�е�С����󷵻ظ�CentralCache
			if (pTLSThreadCache)
				tcPool.Delete(pTLSThreadCache);
		});
	}

	for (auto& t : vthread)
	{
		t.join();
	}

	//printf("%u���̲߳���ִ��%u�ִΣ�ÿ�ִ�concurrent alloc %u��: ���ѣ�%u ms\n",
	//	nworks, rounds, ntimes, malloc_costtime);

	//printf("%u���̲߳���ִ��%u�ִΣ�ÿ�ִ�concurrent dealloc %u��: ���ѣ�%u ms\n",
	//	nworks, rounds, ntimes, free_costtime);

	//printf("%u���̲߳���concurrent alloc&dealloc %u�Σ��ܼƻ��ѣ�%u ms\n",
	//	nworks, nworks*rounds*ntimes, malloc_costtime + free_costtime);

	//printf("%u���̲߳���ִ��%u�ִΣ�ÿ�ִ�malloc %u��: ���ѣ�%u ms\n",
//	nworks, rounds, ntimes, malloc_costtime);

	cout << nworks << "���̲߳���ִ��" << rounds << "�ִ�" << ",ÿ�ִ�concurrent alloc" << ntimes << "��: ���ѣ�" << malloc_costtime << "ms" << endl;

	//printf("%u���̲߳���ִ��%u�ִΣ�ÿ�ִ�free %u��: ���ѣ�%u ms\n",
	//	nworks, rounds, ntimes, free_costtime);

	cout << nworks << "���̲߳���ִ��" << rounds << "�ִ�" << ",ÿ�ִ�concurrent dealloc" << ntimes << "��: ���ѣ�" << free_costtime << "ms" << endl;

	//printf("%u���̲߳���malloc&free %u�Σ��ܼƻ��ѣ�%u ms\n",
	//	nworks, nworks*rounds*ntimes, malloc_costtime + free_costtime);

	cout << nworks << "���̲߳���ִ��concurrent alloc&dealloc " << nworks * rounds*ntimes << "��, �ܼƻ���:��" << malloc_costtime + free_costtime << "ms" << endl;
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
