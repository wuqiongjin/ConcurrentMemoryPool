#include "PageCache.h"

PageCache PageCache::_sInst;	//��̬��Ա���ⶨ��

//NewSpan����Span��3�����:(ÿ��ֻ��CentralCache 1��Span)
//1. CentralCache��Ҫ��ҳ���µ�Spanlist�д���Span����ôֱ�ӷ������1��Span�Ϳ�����
//2. CentralCache��Ҫ��ҳ���µ�Spanlist��û��Span, ��ô�ӵ�ǰλ���������Spanlist[k + 1], �ҵ�Span�󣬶�Span�����з֣�Ȼ���ٹ��أ��������CentralCache
//3. ��ǰλ�������Spanlistû��Span�ˣ���ô��ϵͳ(����)����һ��NPAGES��С��Span��Ȼ���������зֺ͹��أ��������CentralCache
Span* PageCache::NewSpan(size_t page)
{
	//this->PageLock();
	//�ȿ���ǰҳ����λ���Ƿ���Span
	if (!_spanlists[page].Empty())
	{
		Span* span = _spanlists[page].PopFront();
		//this->PageUnLock();	//����������֪��ֻ��ݹ�1�ε�����²�ѡ���ֶ�UnLock()�ģ���Ȼ�Ƽ�ʹ��unique_lock
		//this->PageUnLock();	//�����������, �ݹ�������Σ���Ҫ��������
		return span;
	}

	//��ȥ�鿴��ǰλ���Ժ��λ���Ƿ���Span
	for (int i = page + 1; i < NPAGES; ++i)
	{
		//�ҵ���Span
		if (!_spanlists[i].Empty())
		{
			Span* BigSpan = _spanlists[i].PopFront();
			Span* SmallSpan = new Span;	//newһ��С��span���ڴ������һ���зֺõ�span

			SmallSpan->_n = page;
			SmallSpan->_pageID = BigSpan->_pageID;
			BigSpan->_n -= page;
			//BigSpan->_pageID += (page << PAGE_SHIFT);	//����2
			BigSpan->_pageID += page;					//�޸�2

			//ҲҪ��BigSpan����ҳ��βҳ�浽ӳ����У������ڿ��е�Span���кϲ�����ʱ�ܹ�ͨ��ӳ����ҵ�BigSpan�����кϲ�
			_idSpanMap[BigSpan->_pageID] = BigSpan;
			_idSpanMap[BigSpan->_pageID + BigSpan->_n - 1] = BigSpan;	//������-1,�ٸ����ӿ���

			_spanlists[BigSpan->_n].PushFront(BigSpan);
			//this->PageUnLock();
			//this->PageUnLock();

			//���淵�ظ�CentralCacheʱ��SmallSpan��ÿһҳ��SmallSpan��ӳ���ϵ, ��������SmallSpan�зֳ�С���ڴ��Ȼ��С���ڴ�ͨ��ӳ���ϵ�һ�SmallSpan
			for (size_t i = SmallSpan->_pageID; i < SmallSpan->_pageID + SmallSpan->_n; ++i)
			{
				_idSpanMap[i] = SmallSpan;
			}
			return SmallSpan;
		}
	}

	//����1
	//��ǰλ���Ժ��λ��Ҳû��Span,��ôֱ���������һ���СΪ(NPAGES-1)<<PAGE_SHIFT�Ŀռ�
	//Span* newSpan = (Span*)SystemAlloc(NPAGES - 1);	//���ﴫ��ȥ����ҳ��
	//newSpan->_n = NPAGES - 1;
	//newSpan->_pageID = ((int)newSpan >> PAGE_SHIFT);
	//_spanlists[NPAGES - 1].PushFront(newSpan);
	//return NewSpan(page);	//�ݹ�����Լ�������֮ǰ�Ĵ���(��ȻҲ�����ٴ���һ���Ӻ��������Ӻ���ִ��)

	//�޸�1
	Span* newSpan = new Span;	//Span������new������
	void* ptr = SystemAlloc(NPAGES - 1);	//��ϵͳ����Ŀռ��ǽ���newSpan�е�_pageID��_n���й����
	newSpan->_n = NPAGES - 1;
	newSpan->_pageID = ((PAGE_ID)ptr >> PAGE_SHIFT);
	_spanlists[NPAGES - 1].PushFront(newSpan);
	return NewSpan(page);
}

//��PAGE_IDӳ�䵽һ��Span*��, ��������ͨ��ҳ��ֱ���ҵ���Ӧ��Span*��λ��
Span* PageCache::MapPAGEIDToSpan(PAGE_ID id)
{
	auto it = _idSpanMap.find(id);
	if (it != _idSpanMap.end())
	{
		return it->second;
	}
	return nullptr;	//û�ҵ�ӳ���ϵ, ����nullptr
}

//��useCount��Ϊ0��Span���ظ�PageCache���������ϲ��ɸ����Span
void PageCache::ReleaseSpanToPageCache(Span* span)
{
	//����ǰ+�����ң����Ƿ��п��е�Span�ܽ��кϲ����������ٺϲ���ʱ���ٰ�span���뵽��Ӧλ��
	//1. ��ǰ����
	while (1)
	{
		PAGE_ID prevID = span->_pageID - 1;	//����span��ǰһ��Span�����һҳ��ҳ��
		Span* prevSpan = MapPAGEIDToSpan(prevID);//ͨ��ӳ���ϵ���ҵ�ǰ��һ��ҳ���ڵ�Span����
		if (prevSpan == nullptr)	//�ж�ǰ���Span�Ƿ������(���ܻ���ϵͳ��)[�����ӳ����У���֤���������]
			break;
		if (prevSpan->_isUsed == true)	//ǰ���Span���ڱ�CentralCacheʹ��(���ﲻ��ʹ��orevSpan��useCount��Ϊ�ж����ݣ���ΪuseCount��ֵ�ı仯���ڼ�϶[���з�Spanʱ])
			break;
		if (prevSpan->_n + span->_n > NPAGES - 1)//�ϲ���Ĵ�С>128ʱ, ���ܺϲ�
			break;
		//��ʼ�ϲ�
		span->_pageID = prevSpan->_pageID;
		span->_n += prevSpan->_n;

		_spanlists[prevSpan->_n].Erase(prevSpan);
		delete prevSpan;	//prevSpan����ֱ���ͷ�
		prevSpan = nullptr;
	}
	//2. ������
	while (1)
	{
		PAGE_ID nextID = span->_pageID + span->_n;
		Span* nextSpan = MapPAGEIDToSpan(nextID);
		if (nextSpan == nullptr)
			break;
		if (nextSpan->_isUsed == true)
			break;
		if (nextSpan->_n + span->_n > NPAGES - 1)
			break;
		//��ʼ�ϲ�
		span->_n += nextSpan->_n;

		_spanlists[nextSpan->_n].Erase(nextSpan);
		delete nextSpan;
		nextSpan = nullptr;
	}

	span->_isUsed = false;
	_spanlists[span->_n].PushFront(span);	//���ϲ����Span���뵽spanlist����
	_idSpanMap[span->_pageID] = span;
	_idSpanMap[span->_pageID + span->_n - 1] = span;
}