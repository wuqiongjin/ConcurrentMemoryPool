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

	if (actualNum == 1)
	{
		assert(start == end);
	}
	else
	{
		//��ΪCentralCache�����������ThreadCache������ڴ棬���԰ѱ��β���Ҫ���ڴ��ҵ�ThreadCache�Ķ�Ӧfreelist�ϣ���Ҫ���ڴ���ֱ�ӷ���
		_freelists[index].PushRange(Next(start), end);
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

	int index = SizeClass::Index(size);	//�����Ӧ�ò嵽�ĸ�����������
	_freelists[index].Push(ptr);	//��ptr���ռ�ͷ�嵽��Ӧ������������
}