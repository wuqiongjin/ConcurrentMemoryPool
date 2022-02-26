#include "PageCache.h"

PageCache PageCache::_sInst;	//��̬��Ա���ⶨ��

//NewSpan����Span��3�����:(ÿ��ֻ��CentralCache 1��Span)
//1. CentralCache��Ҫ��ҳ���µ�Spanlist�д���Span����ôֱ�ӷ������1��Span�Ϳ�����
//2. CentralCache��Ҫ��ҳ���µ�Spanlist��û��Span, ��ô�ӵ�ǰλ���������Spanlist[k + 1], �ҵ�Span�󣬶�Span�����з֣�Ȼ���ٹ��أ��������CentralCache
//3. ��ǰλ�������Spanlistû��Span�ˣ���ô��ϵͳ(����)����һ��NPAGES��С��Span��Ȼ���������зֺ͹��أ��������CentralCache
Span* PageCache::NewSpan(size_t page)
{
	this->PageLock();
	//�ȿ���ǰҳ����λ���Ƿ���Span
	if (!_spanlists[page].Empty())
	{
		Span* span = _spanlists[page].PopFront();
		this->PageUnLock();
		this->PageUnLock();
		return span;
	}

	//��ȥ�鿴��ǰλ���Ժ��λ���Ƿ���Span
	for (int i = page + 1; i < NPAGES; ++i)
	{
		//�ҵ���Span
		if (!_spanlists[i].Empty())
		{
			Span* BigSpan = _spanlists[i].PopFront();
			Span* smallSpan = new Span;	//newһ��С��span���ڴ������һ���зֺõ�span

			smallSpan->_n = page;
			smallSpan->_pageID = BigSpan->_pageID;
			BigSpan->_n -= page;
			BigSpan->_pageID += (page << PAGE_SHIFT);

			_spanlists[BigSpan->_n].PushFront(BigSpan);
			this->PageUnLock();
			this->PageUnLock();
			return smallSpan;
		}
	}

	//��ǰλ���Ժ��λ��Ҳû��Span,��ôֱ���������һ���СΪ(NPAGES-1)<<PAGE_SHIFT�Ŀռ�
	Span* newSpan = (Span*)SystemAlloc(NPAGES - 1);	//���ﴫ��ȥ����ҳ��
	newSpan->_n = NPAGES - 1;
	newSpan->_pageID = ((int)newSpan >> PAGE_SHIFT);
	_spanlists[NPAGES - 1].PushFront(newSpan);
	return NewSpan(page);	//�ݹ�����Լ�������֮ǰ�Ĵ���(��ȻҲ�����ٴ���һ���Ӻ��������Ӻ���ִ��)
}
