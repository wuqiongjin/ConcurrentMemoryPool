#include "CentralCache.h"

//��ȡһ���ǿյ�Span
Span* CentralCache::GetOneSpan(SpanList& list, size_t size)
{
	//...
	return nullptr;
}

//��CentralCache��ȡһ������(batchNum)�Ķ����ThreadCache������Ͳ���:start��end
//�ú����ķ���ֵ��actualNum:ʵ���ϻ�ȡ���Ķ�������
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size)
{
	size_t index = SizeClass::Index(size);	//ȷ���ĸ�Ͱ
	_spanlists[index].Lock();	//����index���Ͱ�ˣ�Ҫ�ȼ���

	Span* span = GetOneSpan(_spanlists[index], size);
	start = span->_freelist;
	end = start;
	size_t actualNum = 1;
	while (--batchNum && Next(end) != nullptr)	//--batchNum������end����ƶ�batchNum-1��
	{
		++actualNum;
		end = Next(end);
	}
	span->_freelist = Next(end);
	Next(end) = nullptr;	//��ȡ��start��end��η�Χ�Ŀռ��ˣ�����end��֮��Ŀռ�Ͽ�����

	_spanlists[index].UnLock();
	return actualNum;
}