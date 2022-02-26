#include "CentralCache.h"
#include "PageCache.h"

//static��Ա��Ҫ�����ⶨ��ͳ�ʼ��
CentralCache CentralCache::_sInst;

//��ȡһ���ǿյ�Span
Span* CentralCache::GetOneSpan(SpanList& slist, size_t size)
{
	//��������Span�ҷǿյ�span
	Span* it = slist.Begin();
	while (it != slist.End())
	{
		if (it->_freelist != nullptr)
		{
			return it;	//�ҵ��˷ǿյ�span��ֱ�ӷ���
		}
		it = it->_next;
	}

	//����PageCache��Ҫ֮ǰ�������Ƚ������������߳��ܹ����뵱ǰ��Ͱ��(�������̰߳��ڴ��ͷŻ���)
	slist.UnLock();
	//û�ҵ��ǿյ�span, ��PageCache��Ҫ
	Span* newSpan = PageCache::GetInstance()->NewSpan(SizeClass::SizeToPage(size));	//newSpan�Ǿֲ������������߳̿����������Զ�newSpan�Ĳ�������Ҫ����

	//��newSpan�зֳ�С���ڴ�, ������ЩС���ڴ�ŵ�newSpan�ĳ�Ա_freelist�£�ʹ��Next(obj)������������
	//�ȼ��������ڴ����ʼ��ַ(ҳ��*ÿҳ�Ĵ�С)����ֹ��ַ(ҳ��*ÿҳ�Ĵ�С+��ʼ��ַ)
	char* start = (char*)(newSpan->_pageID << PAGE_SHIFT);
	char* end = start + (newSpan->_n << PAGE_SHIFT);
	
	//_freelist����ø�һ��ͷ��㣬����ͷ��
	newSpan->_freelist = start;

	//��ʼ�з�
	while (start < end)
	{
		Next(start) = start + size;//ÿ��С�ڴ���д��һ��С�ڴ�ĵ�ַ�Դ���߼������ӵ�Ч��
		start += size;
	}

	//PushFront�Ĳ����漰���ٽ����������Ҫ�����������Ĳ�����FetchRangeObj��(��Ϊ����GetOneSpan����ʱ�Ǵ���Ͱ��������!)
	slist.Lock();
	slist.PushFront(newSpan);	//���µ�newSpan���뵽slist��
	return newSpan;
}

//��CentralCache��ȡһ������(batchNum)�Ķ����ThreadCache������Ͳ���:start��end
//�ú����ķ���ֵ��actualNum:ʵ���ϻ�ȡ���Ķ�������
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size)
{
	size_t index = SizeClass::Index(size);	//ȷ���ĸ�Ͱ
	_spanlists[index].Lock();	//����index���Ͱ�ˣ�Ҫ�ȼ���

	Span* span = GetOneSpan(_spanlists[index], size);
	assert(span && span->_freelist);

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