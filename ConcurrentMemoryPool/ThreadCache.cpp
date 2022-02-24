#include "ThreadCache.h"

void* ThreadCache::FetchFromCentralCache(size_t Index, size_t alignNum)
{
	//...
	return nullptr;
}

//��ThreadCache����ռ�
void* ThreadCache::Allocate(size_t size)
{
	assert(size <= MAX_BYTES);
	size_t alignNum = SizeClass::RoundUp(size);	//������(���뵽ÿ�����Ĵ�СΪalignNum������������)
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