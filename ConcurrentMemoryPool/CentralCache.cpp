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
	PageCache::GetInstance()->PageLock();	//�������Ҳû����, �����NewSpan���������, ����Ҫʹ�õݹ�����������
	Span* newSpan = PageCache::GetInstance()->NewSpan(SizeClass::SizeToPage(size));	//newSpan�Ǿֲ������������߳̿����������Զ�newSpan�Ĳ�������Ҫ����
	newSpan->_isUsed = true;	//newSpan�Ǽ��������CentralCacheʹ�õ�Span
	PageCache::GetInstance()->PageUnLock();

	//��newSpan�зֳ�С���ڴ�, ������ЩС���ڴ�ŵ�newSpan�ĳ�Ա_freelist�£�ʹ��Next(obj)������������
	//�ȼ��������ڴ����ʼ��ַ(ҳ��*ÿҳ�Ĵ�С)����ֹ��ַ(ҳ��*ÿҳ�Ĵ�С+��ʼ��ַ)
	char* start = (char*)(newSpan->_pageID << PAGE_SHIFT);
	char* end = start + (newSpan->_n << PAGE_SHIFT);

	//_freelist����ø�һ��ͷ��㣬����ͷ��
	newSpan->_freelist = (void*)start;
	
	//��ʼ�з�
	while (start < end)
	{
		Next(start) = (void*)(start + size);//ÿ��С�ڴ���д��һ��С�ڴ�ĵ�ַ�Դ���߼������ӵ�Ч��
		//start = (char*)Next(start);
		start += size;	//�ȼ��������
	}
	cout << std::this_thread::get_id() << ":" << " start:" << (void*)start << endl;
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

	span->_useCount += actualNum;

	_spanlists[index].UnLock();
	return actualNum;
}

//����start��ʼ��freelist�е�����С���ڴ涼�黹(ͷ��)��spanlists[index]�еĸ��Ե�span
void CentralCache::ReleaseListToSpans(void* start, size_t size)
{
	assert(start);
	size_t index = SizeClass::Index(size);
	_spanlists[index].Lock();

	while (start)
	{
		void* NextPos = Next(start);
		PAGE_ID id = ((PAGE_ID)start >> PAGE_SHIFT);//��ĳһҳ���е����е�ַ����ҳ�Ĵ�С�����ǵõ��Ľ�����ǵ�ǰҳ��ҳ��
		Span* ret = PageCache::GetInstance()->MapPAGEIDToSpan(id);//ӳ���ϵ��PageCache��Span�ָ�CentralCacheʱ���д洢
		//��startС���ڴ�ͷ�嵽Span* ret��_freelist����
		if(ret != nullptr)
		{
			Next(start) = ret->_freelist;
			ret->_freelist = start;
		}
		else
		{
			assert(false);	//��Ӧ����nullptr
		}

		//��useCount��Ϊ0��Span���ظ�PageCache���������ϲ��ɸ����Span
		--ret->_useCount;
		if (ret->_useCount == 0)
		{
			_spanlists[index].UnLock();
			PageCache::GetInstance()->PageLock();
			PageCache::GetInstance()->ReleaseSpanToPageCache(ret);
			PageCache::GetInstance()->PageUnLock();
			_spanlists[index].Lock();
		}

		start = NextPos;
	}
	_spanlists[index].UnLock();
}