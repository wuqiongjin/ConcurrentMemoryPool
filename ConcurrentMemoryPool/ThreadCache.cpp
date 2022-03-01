#include "ThreadCache.h"
#include "CentralCache.h"

//��CentralCache��ȡ�ڴ�
void* ThreadCache::FetchFromCentralCache(size_t index, size_t size)//size����Ҫ�Ŀռ�Ĵ�С(�������ֽ���)
{
	//ʹ��"����ʼ"���������㷨�����CentralCache������Ҫ�������
	//ʹ������ʼ���ﵽ�ļ���Ŀ��:
	//1. ��֤�ʼ��һ�β�����CentralCache��Ҫ̫��,��Ϊ����Է�ֻ����һ��ô�С���ڴ�����Ҳ������,��ʱ����˷�
	//2. ���㲻�ϵ�Ҫ���size��С���ڴ�ʱ��batchNum�ͻ᲻��������ֱ������
	//3. sizeԽ��ʱ, һ����CentralCacheҪ��batchNum�ͻ�ԽС
	//4. sizeԽСʱ��һ����CentralCacheҪ��batchNum�ͻ�Խ��
	size_t batchNum = min(_freelists[index].MaxSize(), SizeClass::SizeToMaxBatchNum(size));
	if (_freelists[index].MaxSize() == batchNum)
	{
		_freelists[index].MaxSize() += 1;
	}

	void* start = nullptr;
	void* end = nullptr;
	size_t actualNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, size);

	////�����ϵ�
	//void* cur = start;
	//int i = 0;
	//while (cur)
	//{
	//	++i;
	//	cur = Next(cur);
	//}
	//if (i != actualNum)
	//{
	//	cout << "error" << endl;
	//}

	if (actualNum == 1)
	{
		assert(start == end);
	}
	else
	{
		//��ΪCentralCache�����������ThreadCache������ڴ棬���԰ѱ��β���Ҫ���ڴ��ҵ�ThreadCache�Ķ�Ӧfreelist�ϣ���Ҫ���ڴ���ֱ�ӷ���
		_freelists[index].PushRange(Next(start), end, actualNum - 1);
	}
	return start;
}

//��ThreadCache����ռ�
void* ThreadCache::Allocate(size_t size)
{
	assert(size <= MAX_BYTES);
	size_t alignNum = SizeClass::RoundUp(size);	//�������ֽ���(���뵽ÿ�����Ĵ�СΪalignNum������������)
	size_t index = SizeClass::Index(size);		//�����ǵڼ�����������(�����±�)

	if (!_freelists[index].Empty())
	{
		return _freelists[index].Pop();	//��freelist�϶�Ӧ��Ͱ�з���һ��ռ�
	}
	else
	{
		return FetchFromCentralCache(index, alignNum);	//��CentralCache������һ��ռ䲢����
	}
}

void ThreadCache::Deallocate(void* ptr, size_t size)
{
	assert(ptr);
	assert(size <= MAX_BYTES);
	
	size_t index = SizeClass::Index(size);	//�����Ӧ�ò嵽�ĸ�����������
	_freelists[index].Push(ptr);	//��ptr���ռ�ͷ�嵽��Ӧ������������

	//����������������ŵ�С���ڴ������ >= һ�����������С���ڴ������ʱ,
	//���ǽ�Size()��С��С���ڴ�ȫ�����ظ�CentralCache��Span��
	if (_freelists[index].Size() >= _freelists[index].MaxSize())
	{
		//_freelists[index].Print();
		ListTooLong(_freelists[index], size);
	}
}


void ThreadCache::ListTooLong(FreeList& freelist, size_t size)
{
	//int index = SizeClass::Index(size);	//����CentralCache�е�Ͱ���±�
	void* start = nullptr;
	void* end = nullptr;
	
	freelist.PopRange(start, end, freelist.MaxSize());	//��freelist Pop��һ������(����ĵ���������ûʲô����, ���Կ���ɾ��)

	CentralCache::GetInstance()->ReleaseListToSpans(start, size);
}