#pragma once
#include "Common.h"
#include "ObjectPool.h"
#include "PageMap.h"


//����ģʽPageCache
class PageCache
{
public:
	static PageCache* GetInstance()
	{
    return &_sInst;
    //static std::mutex mtx;
    //if(_sInst == nullptr)
    //{
    //  mtx.lock();
    //  if(_sInst == nullptr)
    //  {
    //    cout << "new a PageCache()" << endl;
    //    _sInst = new PageCache;
    //    if(_sInst == nullptr)
    //      cout << "new PageCache() failed!" << endl;
    //    if(_sInst != nullptr)
    //      cout << "new PageCache() Success!" << endl;
    //  }
    //  mtx.unlock();
    //}
    //return _sInst;
	}

	//�������һ��"ҳ��"��СΪ"NPAGES - 1"���µ�Span
	Span* NewSpan(size_t page);

	//��PAGE_IDӳ�䵽һ��Span*��, ��������ͨ��ҳ��ֱ���ҵ���Ӧ��Span*��λ��
	Span* MapPAGEIDToSpan(PAGE_ID id);

	//��useCount��Ϊ0��Span���ظ�PageCache���������ϲ��ɸ����Span
	void ReleaseSpanToPageCache(Span* span);

	void PageLock() { _pageMutex.lock(); }
	void PageUnLock() { _pageMutex.unlock(); }
private:
	PageCache() {}
	PageCache(const PageCache&) = delete;
	PageCache& operator=(const PageCache&) = delete;

	static PageCache _sInst;
	std::recursive_mutex _pageMutex;	//һ�Ѵ�����һ������PageCache��Ҫ����

private:
	SpanList _spanlists[NPAGES];	//��ҳ��Ϊӳ��Ĺ���(ֱ�Ӷ�ַ��)
	//std::unordered_map<PAGE_ID, Span*> _idSpanMap;
	//TCMalloc_PageMap1<BITSNUM> _idSpanMap;
	TCMalloc_PageMap3<BITSNUM> _idSpanMap;
	ObjectPool<Span> _spanPool;
};
