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
	void* ptr = VirtualAlloc(0, bytes, MEM_COMMIT | MEM_RESERVE,	//ֱ����ϵͳ����ռ�
		PAGE_READWRITE);
#else
	// linux��brk mmap��
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
		if (_freeList)	//freeList������п��пռ�ʱ�����ȴ�freeList����ȡ
		{
			obj = (T*)_freeList;
			_freeList = *(void**)_freeList;	//��free	List��ת����һ������
		}
		else
		{
			if (_remainBytes < sizeof(T))
			{
				_remainBytes = 128 * 1024;	//���_remainBytes<sizeof(T)���Ҳ�Ϊ0ʱ���ӵ��ⲿ�ֿռ䣬��������
				_memory = (char*)SystemAlloc(_remainBytes);
				if (_memory == nullptr)
				{
					throw std::bad_alloc();
				}
			}

			obj = (T*)_memory;
			//�������Ĵ�СС��ָ��Ĵ�С����ô����ҲҪÿ�θ�����������ָ��Ĵ�С����ΪfreeList�����ܴ����һ��ָ��Ĵ�С
			size_t objSize = sizeof(T) > sizeof(void*) ? sizeof(T) : sizeof(void*);
			_memory += objSize;
			_remainBytes -= objSize;
		}
		
		//ʹ�ö�λnew����obj����Ĺ��캯��
		new(obj)T;
		return obj;
	}

	void Delete(T* ptr)
	{
		ptr->~T();	//�ֶ�����T�������������
		//ͷ�巨�������õĿռ�嵽freeList����
		*(void**)ptr = _freeList;	//��32λ�£�*(void**)�ܿ���4���ֽڣ���64λ�£�*(void**)�ܿ���8���ֽ�
		_freeList = ptr;
	}

private:
	char* _memory = nullptr;	//ָ���ڴ���ָ��
	size_t _remainBytes = 0;	//�ڴ���ʣ��ռ��ֽ���
	void* _freeList = nullptr;	//������ڴ�������ͷų����Ŀռ����������
};

//����
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
	// �����ͷŵ��ִ�
	const size_t Rounds = 3;
	// ÿ�������ͷŶ��ٴ�
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